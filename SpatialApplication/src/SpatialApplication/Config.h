//
//      ██╗██████╗     ██╗     ██╗██████╗ ███████╗
//      ██║██╔══██╗    ██║     ██║██╔══██╗██╔════╝		** JPL Spatial Application **
//      ██║██████╔╝    ██║     ██║██████╔╝███████╗
// ██   ██║██╔═══╝     ██║     ██║██╔══██╗╚════██║		https://github.com/Jaytheway/JPLSpatialApplication
// ╚█████╔╝██║         ███████╗██║██████╔╝███████║
//  ╚════╝ ╚═╝         ╚══════╝╚═╝╚═════╝ ╚══════╝
//
//   Copyright 2026 Jaroslav Pevno, JPL Spatial Application is offered under the terms of the ISC license:
//
//   Permission to use, copy, modify, and/or distribute this software for any purpose with or
//   without fee is hereby granted, provided that the above copyright notice and this permission
//   notice appear in all copies. THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
//   WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
//   AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR
//   CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
//   WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
//   CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

#pragma once

/// This header contains mostly development macros to enable/disable certain parts of the code

// Enable air absorption GUI
#define JPL_DEV_AIR_ABSORPTION_GUI 0

// Enabled GUI for manual and deterministic BDPT controls.
// This also disables runtime updates for BDPT.
#define JPL_DEV_BDPT_DBG_GUI 0

// Bypass send direct sound to late reverb, omitting early reflections
#define JPL_DEV_DIRECT_TO_LR 0

// Integrate air absorption in BDPT trace step to include its contribution
// to the resulting energy response.
// The alternative is to add air absorption dB/s contribution to the
// resulting slope after the trace.
#define JPL_DEV_BDPT_AIRABS_INTEGRATED 0

// Temporarily disabling unfinished Bidirectional Path Tracing
#define JPL_DEV_HAS_BDPT 0

// Enable Late Reverb preview impulse trigger and "Trigger Impulse" GUI
#define JPL_DEV_ENABLE_REV_PREVIEW 0
