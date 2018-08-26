//          Copyright Edvard Severin Pettersen 2017.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

// Transforms a (version, revision, patchlevel) triple into a number of the
// form 0xVVRRPPPP to allow comparing versions in a normalized way.
//
//See http://sourceforge.net/p/predef/wiki/VersionNormalization.
#define PROXC_CONFIG_VERSION(version, revision, patch) \
    (((version) << 24) + ((revision) << 16) + (patch))

// Macro expanding to the major version of the library, i.e. the `x` in `x.y.z`.
#define PROXC_MAJOR_VERSION 0

// Macro expanding to the major version of the library, i.e. the `x` in `x.y.z`.
#define PROXC_MINOR_VERSION 1

// Macro expanding to the major version of the library, i.e. the `x` in `x.y.z`.
#define PROXC_PATCH_VERSION 0

// Macro expanding to the full version of the library, in hexadecimal
// representation.
#define PROXC_VERSION                         \
    PROXC_CONFIG_VERSION(PROXC_MAJOR_VERSION, \
                         PROXC_MINOR_VERSION, \
                         PROXC_PATCH_VERSION)

