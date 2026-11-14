/*
Fiber internal header
Copyright (C)2006 bo Ondrej Novak
http://bredy.jinak.cz
novak.on@centrum.cz


This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
#ifndef _MULTITHREAD_COMMON_FIBER_H_
#define _MULTITHREAD_COMMON_FIBER_H_

namespace MultiThread
{
     ///Sets fiber defaults.
    /**
     * This function is for Linux environment. If you want to use it, include src-linux/Fiber.h into your sources
     *
     * @param min Minimum size for fiber stack. Any request below this limit will replaced by this value.
     *               Use 0 to leave this parameter unchanged. Default value is 4KB.
     * @param def Default size for fiber stack. Parameter is applied, when fiber requests zero sized stack
     *               Use 0 to leave this parameter unchanged. Default value is 64KB.
     * @param limit Maximum size for fiber stack. Any request above this limit will replaced by this value.
     *               Use 0 to leave this parameter unchanged. Default value is "no limit".
     *  
     */
 
    void Fiber_SetStackParameters(size_t min, size_t def, size_t limit);
}
#endif /*FIBER_H_*/
