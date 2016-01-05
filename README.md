# root2hdf5

Liberate your data from the monstrosity that *is* ROOT.

This tool is currently incomplete.  Unfortunately the ROOT file format is so
inextricably linked to C++ data structures and semantics that I doubt a flawless
conversion is possible.  Most scalar branch types are supported however, so for
flat ntuples this tool might be useful.


## Requirements

- CMake (for building) 2.8.3+
- Boost 1.50.0+
- ROOT (any relatively recent version)
- HDF5 1.8+


## Acknowledgements

FindROOT.cmake used under LGPL license from the ROOT source code:

    Copyright (C) 2013 CERN

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
