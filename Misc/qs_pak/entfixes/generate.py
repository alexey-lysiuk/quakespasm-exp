#!/usr/bin/env python3

import difflib
import os
import pathlib


header = b'''/*

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
'''


def _main():
    self_path = pathlib.Path(__file__).cwd()
    original_path = self_path / 'original'
    fixed_path = self_path / 'fixed'

    patchindex = 0
    fixes = []

    output = open(self_path / '../../../Quake/entfixes.h', 'wb')
    output.write(header)
    output.write(b'\nstatic constexpr EF_Patch ef_patches[] =\n{\n')

    for filename in os.listdir(original_path):
        print(filename)

        if patchindex > 0:
            output.write(b'\n')

        basename = os.path.basename(filename)
        basenamenoext = os.path.splitext(basename)[0].encode('ascii')
        output.write(b'\t// %s\n' % basenamenoext)

        original = open(original_path / filename, 'rb').read()
        fixed = open(fixed_path / filename, 'rb').read()
        opcodes = difflib.SequenceMatcher(a=original, b=fixed).get_opcodes()

        patchcount = 0

        for opcode in opcodes:
            operation = opcode[0]

            if operation == 'equal':
                begin = opcode[1]
                end = opcode[2]
                output.write(b'\t{ EF_COPY, %u, %u },\n' % (begin, end - begin))
                patchcount += 1
            elif operation == 'insert' or operation == 'replace':
                begin = opcode[3]
                end = opcode[4]
                data = fixed[begin:end]
                data = data.replace(b'\n', rb'\n')
                data = data.replace(b'"', rb'\"')
                output.write(b'\t{ EF_INSERT, 0, %u, "%s" },\n' % (end - begin, data))
                patchcount += 1

        mapname, crc = basenamenoext.split(b'@')
        fixes.append((mapname, len(original) + 1, len(fixed) + 1, crc.upper(), patchindex, patchcount))

        patchindex += patchcount

    output.write(b'};\n\nstatic constexpr EF_Fix ef_fixes[] =\n{\n')

    for fix in fixes:
        output.write(b'\t{ "%s", %i, %i, 0x%s, %i, %i },\n' % fix)

    output.write(b'};\n')


if __name__ == '__main__':
    _main()
