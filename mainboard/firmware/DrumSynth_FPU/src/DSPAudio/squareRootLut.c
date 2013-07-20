/*
 * squareRootLut.c
 *
 *  Created on: 22.06.2012
 * ------------------------------------------------------------------------------------------------------------------------
 *  Copyright 2013 Julian Schmidt
 *  Julian@sonic-potions.com
 * ------------------------------------------------------------------------------------------------------------------------
 *  This file is part of the Sonic Potions LXR drumsynth firmware.
 * ------------------------------------------------------------------------------------------------------------------------
 *  Redistribution and use of the LXR code or any derivative works are permitted
 *  provided that the following conditions are met:
 *
 *       - The code may not be sold, nor may it be used in a commercial product or activity.
 *
 *       - Redistributions that are modified from the original source must include the complete
 *         source code, including the source code for all components used by a binary built
 *         from the modified sources. However, as a special exception, the source code distributed
 *         need not include anything that is normally distributed (in either source or binary form)
 *         with the major components (compiler, kernel, and so on) of the operating system on which
 *         the executable runs, unless that component itself accompanies the executable.
 *
 *       - Redistributions must reproduce the above copyright notice, this list of conditions and the
 *         following disclaimer in the documentation and/or other materials provided with the distribution.
 * ------------------------------------------------------------------------------------------------------------------------
 *   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 *   INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *   DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 *   SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *   WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 *   USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * ------------------------------------------------------------------------------------------------------------------------
 */



#include "squareRootLut.h"

const float squareRootLut[128] =
{
0, 0.0887356, 0.125491, 0.153695, 0.177471,
0.198419, 0.217357, 0.234772, 0.250982, 0.266207,
0.280607, 0.294303, 0.307389, 0.319941, 0.332018,
0.343672, 0.354943, 0.365866, 0.376473, 0.38679,
0.396838, 0.406638, 0.416207, 0.425561, 0.434714,
0.443678, 0.452465, 0.461084, 0.469545, 0.477856,
0.486025, 0.494059, 0.501965, 0.509748, 0.517413,
0.524967, 0.532414, 0.539758, 0.547003, 0.554154,
0.561214, 0.568185, 0.575073, 0.581879, 0.588606,
0.595257, 0.601834, 0.608341, 0.614779, 0.62115,
0.627456, 0.633699, 0.639882, 0.646005, 0.652071,
0.658081, 0.664037, 0.669939, 0.675791, 0.681591,
0.687343, 0.693048, 0.698705, 0.704317, 0.709885,
0.71541, 0.720892, 0.726333, 0.731733, 0.737094,
0.742416, 0.7477, 0.752947, 0.758158, 0.763333,
0.768473, 0.773579, 0.778652, 0.783692, 0.7887,
0.793676, 0.798621, 0.803535, 0.80842, 0.813276,
0.818102, 0.822901, 0.827671, 0.832414, 0.83713,
0.84182, 0.846484, 0.851122, 0.855736, 0.860324,
0.864888, 0.869428, 0.873945, 0.878438, 0.882909,
0.887357, 0.891782, 0.896186, 0.900568, 0.90493,
0.90927, 0.913589, 0.917889, 0.922168, 0.926427,
0.930667, 0.934888, 0.93909, 0.943273, 0.947437,
0.951584, 0.955712, 0.959823, 0.963916, 0.967992,
0.97205, 0.976092, 0.980117, 0.984126, 0.988118,
0.992095, 0.996055, 1};
