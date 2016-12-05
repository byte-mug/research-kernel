/*
 * 
 * Copyright (c) 2016 Simon Schmidt
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#pragma once

enum {
	/*
	 * Indicates, that this port is alive (which means, it boots).
	 */
	platform_ALIVE = 0,
	/*
	 * Indicates, that this port is already a "Higher Half Kernel".
	 */
	platform_HIGHER_HALF = 1,
	/*
	 * Indicates, that this port has a working 'struct cpu'-pointer,
	 * including 'struct kernslice'-instance and other accessoires.
	 */
	platform_CPU_PTR = 2,
	/*
	 * Indicates, that this port has working MMU suport.
	 */
	platform_MMU = 3,
	/*
	 * Indicates, that this port supports Interrupts.
	 */
	platform_INTERRUPTS = 4,
};

/*
 * returns one of these platform_* constants, defined before.
 */
int platform_get_cap_stage();


