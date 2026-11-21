use pbo_codec::decode;

#[test]
fn decodes_literal_bytes_and_checksum() {
    let compressed = [0x07, b'A', b'B', b'C', 0xC6, 0x00, 0x00, 0x00];
    let mut output = [0xCC; 3];

    assert!(decode(&compressed, &mut output).is_ok());
    assert_eq!(&output, b"ABC");
}

#[test]
fn decodes_overlapping_back_reference() {
    let compressed = [0x01, b'A', 0x01, 0x00, 0x04, 0x01, 0x00, 0x00];
    let mut output = [0; 4];

    assert!(decode(&compressed, &mut output).is_ok());
    assert_eq!(&output, b"AAAA");
}

#[test]
fn decodes_back_reference_across_window_wrap() {
    let mut compressed = vec![0xFF];
    compressed.extend_from_slice(b"AAAAAAAA");
    compressed.push(0xFF);
    compressed.extend_from_slice(b"AAAAAAAA");
    compressed.extend_from_slice(&[0x03, b'A', b'A', 0x01, 0x00]);
    compressed.extend_from_slice(&[0x55, 0x05, 0x00, 0x00]);
    let mut output = [0; 21];

    assert!(decode(&compressed, &mut output).is_ok());
    assert_eq!(&output, b"AAAAAAAAAAAAAAAAAAAAA");
}

#[test]
fn rejects_truncated_literal() {
    let error = decode(&[0x01], &mut [0; 1]).unwrap_err();
    assert_eq!(format!("{error:?}"), "TruncatedInput");
}

#[test]
fn rejects_every_truncated_prefix_and_decodes_next_block() {
    let compressed = [0x07, b'A', b'B', b'C', 0xC6, 0x00, 0x00, 0x00];
    for prefix_len in 0..compressed.len() {
        let error = decode(&compressed[..prefix_len], &mut [0; 3]).unwrap_err();
        assert_eq!(
            format!("{error:?}"),
            "TruncatedInput",
            "prefix {prefix_len}"
        );
    }

    let mut output = [0; 3];
    assert!(decode(&compressed, &mut output).is_ok());
    assert_eq!(&output, b"ABC");
}

#[test]
fn decodes_independent_blocks_concurrently() {
    let compressed = vec![0x07, b'A', b'B', b'C', 0xC6, 0x00, 0x00, 0x00];
    let workers: Vec<_> = (0..4)
        .map(|_| {
            let block = compressed.clone();
            std::thread::spawn(move || {
                let mut output = [0; 3];
                assert!(decode(&block, &mut output).is_ok());
                assert_eq!(&output, b"ABC");
            })
        })
        .collect();

    for worker in workers {
        worker.join().expect("decoder worker panicked");
    }
}

#[test]
fn rejects_back_reference_larger_than_destination() {
    let error = decode(&[0x00, 0xEE, 0xF0], &mut [0; 1]).unwrap_err();
    assert_eq!(format!("{error:?}"), "OutputOverflow");
}

#[test]
fn rejects_checksum_mismatch() {
    let error = decode(&[0x01, b'A', 0x00, 0x00, 0x00, 0x00], &mut [0; 1]).unwrap_err();
    assert_eq!(format!("{error:?}"), "ChecksumMismatch");
}

#[test]
fn accepts_zero_length_output_without_reading_input() {
    assert!(decode(&[], &mut []).is_ok());
}

#[test]
fn reports_abi_version_one() {
    assert_eq!(pbo_codec::dayz_pbo_abi_version(), 1);
}

#[test]
fn c_abi_decodes_into_the_requested_output_range() {
    let compressed = [0x07, b'A', b'B', b'C', 0xC6, 0, 0, 0];
    let mut guarded_output = [0xCC; 5];
    let status = unsafe {
        pbo_codec::dayz_pbo_decode_v1(
            compressed.as_ptr(),
            compressed.len(),
            guarded_output.as_mut_ptr().add(1),
            3,
        )
    };

    assert_eq!(status, 0);
    assert_eq!(guarded_output, [0xCC, b'A', b'B', b'C', 0xCC]);
}

#[test]
fn c_abi_rejects_null_and_overlapping_nonempty_buffers() {
    let mut output = [0; 1];
    let null_input_status =
        unsafe { pbo_codec::dayz_pbo_decode_v1(std::ptr::null(), 1, output.as_mut_ptr(), 1) };
    let null_output_status =
        unsafe { pbo_codec::dayz_pbo_decode_v1([0x01].as_ptr(), 1, std::ptr::null_mut(), 1) };
    let mut shared = [0; 8];
    let shared_pointer = shared.as_mut_ptr();
    let overlapping_status =
        unsafe { pbo_codec::dayz_pbo_decode_v1(shared_pointer, 4, shared_pointer.add(1), 3) };

    assert_eq!(null_input_status, 1);
    assert_eq!(null_output_status, 1);
    assert_eq!(overlapping_status, 1);
}

#[test]
fn c_abi_maps_decoder_errors_to_stable_status_codes() {
    let truncated_status =
        unsafe { pbo_codec::dayz_pbo_decode_v1([0x01].as_ptr(), 1, [0].as_mut_ptr(), 1) };
    let overflow_status = unsafe {
        pbo_codec::dayz_pbo_decode_v1([0x00, 0x01, 0x00].as_ptr(), 3, [0].as_mut_ptr(), 1)
    };
    let checksum_status = unsafe {
        pbo_codec::dayz_pbo_decode_v1([0x01, b'A', 0, 0, 0, 0].as_ptr(), 6, [0].as_mut_ptr(), 1)
    };

    assert_eq!(truncated_status, 2);
    assert_eq!(overflow_status, 3);
    assert_eq!(checksum_status, 4);
}

#[test]
fn c_abi_accepts_null_pointers_for_empty_buffers() {
    let status =
        unsafe { pbo_codec::dayz_pbo_decode_v1(std::ptr::null(), 0, std::ptr::null_mut(), 0) };

    assert_eq!(status, 0);
}

#[test]
fn c_abi_rejects_lengths_that_cannot_form_rust_slices() {
    let byte = 0;
    let status = unsafe {
        pbo_codec::dayz_pbo_decode_v1(&byte, isize::MAX as usize + 1, std::ptr::null_mut(), 0)
    };

    assert_eq!(status, 1);
}
