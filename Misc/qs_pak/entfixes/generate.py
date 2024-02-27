#!/usr/bin/env python3

import difflib
import os
import pathlib


def _main():
    self_path = pathlib.Path(__file__)
    original_path = self_path.parent / 'original'
    fixed_path = self_path.parent / 'fixed'

#    for filename in os.listdir(original_path):
#        print(filename)
#        original = open(original_path / filename, 'rb').read()
#        fixed = open(fixed_path / filename, 'rb').read()
#        opcodes = difflib.SequenceMatcher(a=original, b=fixed).get_opcodes()
#        for opcode in opcodes:
#            print(opcode)
#        break

    output = open('entfixes.h', 'wb')
    output.write(b'static constexpr EF_Patch ef_patches[] =\n{\n')

    # filename = 'e1m1@c49d.ent'
    # filename = 'hrim_sp1@f054.ent'
    filename = 'e2m2@fbfe.ent'
    basename = os.path.basename(filename)
    basenamenoext = os.path.splitext(basename)[0].encode('ascii')
    output.write(b'\t// %s\n' % basenamenoext)

    original = open(original_path / filename, 'rb').read()
    fixed = open(fixed_path / filename, 'rb').read()
    opcodes = difflib.SequenceMatcher(a=original, b=fixed).get_opcodes()
    for opcode in opcodes:
        # print(opcode)
        operation = opcode[0]

        if operation == 'equal':
            begin = opcode[1]
            end = opcode[2]
            # print(f'{{ EF_COPY, {begin}, {end - begin} }},')
            output.write(b'\t{ EF_COPY, %u, %u },\n' % (begin, end - begin))
            # output.write(hex(begin).encode('ascii'))
            # , {end - begin} }},')
        elif operation == 'insert' or operation == 'replace':
            begin = opcode[3]
            end = opcode[4]
            data = fixed[begin:end]  # .decode('ascii')
            data = data.replace(b'\n', rb'\n')
            data = data.replace(b'"', rb'\"')
            # print(f'{{ EF_INSERT, 0, {end - begin}, "{data}" }},')
            output.write(b'\t{ EF_INSERT, 0, %u, "%s" },\n' % (end - begin, data))
        # elif opcode[0] == 'replace':

    output.write(b'};')


if __name__ == '__main__':
    _main()
