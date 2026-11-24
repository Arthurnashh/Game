use std::panic::{catch_unwind, AssertUnwindSafe};
use std::slice;

const WINDOW_SIZE: usize = 4096;
const LOOKAHEAD_SIZE: usize = 18;
const THRESHOLD: usize = 2;
const WINDOW_MASK: usize = WINDOW_SIZE - 1;

#[derive(Debug, PartialEq, Eq)]
pub enum DecodeError {
    TruncatedInput,
    OutputOverflow,
    ChecksumMismatch,
}

fn read_byte(src: &[u8], position: &mut usize) -> Result<u8, DecodeError> {
    let byte = src
        .get(*position)
        .copied()
        .ok_or(DecodeError::TruncatedInput)?;
    *position += 1;
    Ok(byte)
}

/// Decodes one SSCompress block into a caller-owned output buffer.
///
/// A zero-length destination matches the legacy decoder and succeeds without
/// consuming or validating the input. On any error, callers must discard the
/// partially written destination.
pub fn decode(src: &[u8], dst: &mut [u8]) -> Result<(), DecodeError> {
    if dst.is_empty() {
        return Ok(());
    }

    let mut window = [0u8; WINDOW_SIZE + LOOKAHEAD_SIZE - 1];
    window[..WINDOW_SIZE - LOOKAHEAD_SIZE].fill(b' ');

    let mut source_position = 0;
    let mut output_position = 0;
    let mut window_position = WINDOW_SIZE - LOOKAHEAD_SIZE;
    let mut flags = 0u16;
    let mut checksum = 0u32;

    while output_position < dst.len() {
        flags >>= 1;
        if flags & 0x0100 == 0 {
            flags = u16::from(read_byte(src, &mut source_position)?) | 0xFF00;
        }

        if flags & 1 != 0 {
            let byte = read_byte(src, &mut source_position)?;
            dst[output_position] = byte;
            output_position += 1;
            checksum = checksum.wrapping_add(u32::from(byte));
            window[window_position] = byte;
            window_position = (window_position + 1) & WINDOW_MASK;
        } else {
            let low = usize::from(read_byte(src, &mut source_position)?);
            let high = usize::from(read_byte(src, &mut source_position)?);
            let source_window_position = low | ((high & 0xF0) << 4);
            let count = (high & 0x0F) + THRESHOLD + 1;
            if count > dst.len() - output_position {
                return Err(DecodeError::OutputOverflow);
            }

            let mut copy_position =
                (window_position + WINDOW_SIZE - source_window_position) & WINDOW_MASK;
            for _ in 0..count {
                let byte = window[copy_position];
                dst[output_position] = byte;
                output_position += 1;
                checksum = checksum.wrapping_add(u32::from(byte));
                window[window_position] = byte;
                window_position = (window_position + 1) & WINDOW_MASK;
                copy_position = (copy_position + 1) & WINDOW_MASK;
            }
        }
    }

    let expected_checksum = u32::from_le_bytes([
        read_byte(src, &mut source_position)?,
        read_byte(src, &mut source_position)?,
        read_byte(src, &mut source_position)?,
        read_byte(src, &mut source_position)?,
    ]);
    if checksum != expected_checksum {
        return Err(DecodeError::ChecksumMismatch);
    }

    Ok(())
}

#[no_mangle]
pub extern "C" fn dayz_pbo_abi_version() -> u32 {
    1
}

/// Decodes an SSCompress block across the C ABI.
///
/// # Safety
/// For a non-empty buffer whose length does not exceed `isize::MAX`, its
/// pointer must be valid for the stated length. Lengths above `isize::MAX`
/// are rejected before either pointer is dereferenced. The input and output
/// ranges must not overlap. The caller owns both buffers and must discard the
/// destination after any nonzero result.
#[no_mangle]
pub unsafe extern "C" fn dayz_pbo_decode_v1(
    src: *const u8,
    src_len: usize,
    dst: *mut u8,
    dst_len: usize,
) -> i32 {
    if src_len > isize::MAX as usize || dst_len > isize::MAX as usize {
        return 1;
    }

    if (src_len != 0 && src.is_null()) || (dst_len != 0 && dst.is_null()) {
        return 1;
    }

    let src_end = match (src as usize).checked_add(src_len) {
        Some(end) => end,
        None => return 1,
    };
    let dst_end = match (dst as usize).checked_add(dst_len) {
        Some(end) => end,
        None => return 1,
    };
    if src_len != 0 && dst_len != 0 && (src as usize) < dst_end && (dst as usize) < src_end {
        return 1;
    }

    let input = if src_len == 0 {
        &[]
    } else {
        // SAFETY: validity for src_len bytes is part of this function's contract.
        unsafe { slice::from_raw_parts(src, src_len) }
    };
    let output = if dst_len == 0 {
        &mut []
    } else {
        // SAFETY: validity for dst_len writable bytes is part of this function's contract.
        unsafe { slice::from_raw_parts_mut(dst, dst_len) }
    };

    match catch_unwind(AssertUnwindSafe(|| decode(input, output))) {
        Ok(Ok(())) => 0,
        Ok(Err(DecodeError::TruncatedInput)) => 2,
        Ok(Err(DecodeError::OutputOverflow)) => 3,
        Ok(Err(DecodeError::ChecksumMismatch)) => 4,
        Err(_) => 5,
    }
}
