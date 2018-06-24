#!/usr/bin/env python3

import os, sys
import re
from enum import IntEnum
from collections import namedtuple
from jinja2 import Template
from argparse import ArgumentParser, ArgumentTypeError


class Tok(IntEnum):
    '''The type of a `Token`'''
    ERROR = -1
    EOF = -2
    STRING = 0
    NAME = 1
    FIELD_NAME = 2
    FIELD_TYPE = 3
    LBRACE = 10
    RBRACE = 11
    SEMICOLON = 12
    KWD_COMP = 20

class Token:
    '''A lexed token in a .comp file'''

    def __init__(self, type, value=None):
        '''Initializes the token given its `Tok` type and (optional) value'''
        self.type = type
        self.value = value

    def __repr__(self):
        self_str = str(self.type.name)
        if self.value is not None:
            self_str += '(' + str(self.value) + ')'
        return self_str

class CompField:
    '''A field in a comp class '''

    def __init__(self, name, type, descr):
        '''Initializes the field given its name, Ares type and description'''
        self.name = name
        self.type = type
        self.descr = descr

    def __repr__(self):
        return self.name + ': ' + self.type

class Comp:
    '''A comp class'''

    def __init__(self, name, descr, fields):
        '''Initializes the comp class given its name, description and list of fields'''
        self.name = name
        self.descr = descr
        self.fields = list(fields)

    def __repr__(self):
        return self.name + repr(self.fields)


# The list of Ares types that are valid datatypes for comp fields.
COMP_FIELD_TYPES = [
    'I8',
    'I16',
    'I32',
    'I64',
    'U8',
    'U16',
    'U32',
    'U64',
    'F32',
    'F64',
    'Bool',
    'Char',
    'Vec2',
    'Vec3',
    'Vec4',
    'Mat3',
    'Mat4',
    'Str4',
    'Str8',
    'Str16',
    'Str32',
    'Res<[A-Z][A-Za-z0-9]*>',
]

# The regex scanner for a .comp file
COMP_SCANNER = re.Scanner([
    (r'"(\\"|[^"])*"', lambda sc, match: Token(Tok.STRING, match[1:-1])),
    ('|'.join(COMP_FIELD_TYPES), lambda sc, match: Token(Tok.FIELD_TYPE, match)),
    (r'comp', lambda sc, match: Token(Tok.KWD_COMP)),
    (r'[a-z][A-Za-z0-9]+', lambda sc, match: Token(Tok.FIELD_NAME, match)),
    (r'[A-Z][A-Za-z0-9]+', lambda sc, match: Token(Tok.NAME, match)),
    (r'\{', lambda sc, match: Token(Tok.LBRACE)),
    (r'\}', lambda sc, match: Token(Tok.RBRACE)),
    (r';', lambda sc, match: Token(Tok.SEMICOLON)),
    (r'#.*$', None),  # (skip comments)
    (r'\s+', None),  # (skip whitespace)
], re.MULTILINE)

# The extension of .comp files
COMP_EXT = '.comp'

# The path to the .hh.in template for Comp headers
COMP_HH_TEMPLATE_PATH = os.path.join(os.path.dirname(__file__), 'Comp.hh.in')

# The .hh.in template for Comp headers
COMP_HH_TEMPLATE = Template(open(COMP_HH_TEMPLATE_PATH, 'r').read())


def lex(comp_source):
    '''Lexes a .comp file, given its source, to a list of `Token`s.'''

    tokens, rest = COMP_SCANNER.scan(comp_source)
    if rest:
        tokens.append(Token(Tok.ERROR, rest))

    tokens.append(Token(Tok.EOF, None))
    return tokens

def parse(tokens):
    '''Yields `Comp`s as they are parsed from the tokens `lex()`ed from a .comp file.'''

    i = 0

    def expect_tok(type, descr):
        nonlocal i
        if tokens[i].type != type:
            raise SyntaxError('Expected {}, found {}'.format(descr, tokens[i]))
        i += 1
        return tokens[i - 1]

    while i < len(tokens):
        comp = Comp('', '', [])

        expect_tok(Tok.KWD_COMP, '`comp` keyword')
        comp.name = expect_tok(Tok.NAME, 'comp name (PascalCase)').value
        comp.descr = expect_tok(Tok.STRING, 'comp description (string)').value
        expect_tok(Tok.LBRACE, '{')

        while tokens[i].type != Tok.RBRACE:
            field = CompField('', '', '')

            field.type = expect_tok(Tok.FIELD_TYPE, 'comp field type (one of: ' + ', '.join(COMP_FIELD_TYPES) + ')').value
            field.name = expect_tok(Tok.FIELD_NAME, 'comp field name (pascalCase)').value
            field.descr = expect_tok(Tok.STRING, 'comp field description (string)').value

            expect_tok(Tok.SEMICOLON, ';')
            comp.fields.append(field)

            if tokens[i].type == Tok.RBRACE:
                i += 1
                yield comp; break  # end of comp block

        if tokens[i].type == Tok.EOF:
            i += 1
            break  # end of file
        else:
            continue  # parse next comp block

def file_is_newer(path_a, path_b):
    '''Returns `True` if the file at `path_a` is newer than the one at `path_b`,
    or if no file at `path_b` exists'''
    mtime_a = os.path.getmtime(path_a)

    try:
        mtime_b = os.path.getmtime(path_b)
    except OSError:
        return True  # File B does not exist

    return mtime_a > mtime_b

def gen_comp_header(inpath, outpath):
    '''Generates a .hh file at outpath from a .comp file at inpath'''

    with open(inpath, 'r') as infile:
        comp_source = infile.read()

    lexed = lex(comp_source)
    comps = list(parse(lexed))

    with open(outpath, 'w') as outfile:
        outfile.write(COMP_HH_TEMPLATE.render(comps=comps))

def gen_all_comp_headers(indir, outdir):
    '''Generates .hh files in outdir from changed .comp files in indir'''

    for cur_indir, dirnames, filenames in os.walk(indir):
        cur_outdir = os.path.join(outdir, os.path.relpath(cur_indir, indir))
        os.makedirs(cur_outdir, exist_ok=True)

        for filename in filenames:
            name, ext = os.path.splitext(filename)
            if ext != COMP_EXT:
                continue

            inpath = os.path.normpath(os.path.join(cur_indir, filename))
            outpath = os.path.normpath(os.path.join(cur_outdir, name + '.hh'))

            if file_is_newer(inpath, outpath):
                print('-- Generating', outpath)
                gen_comp_header(inpath, outpath)

def read_args(args=sys.argv[1:]):
    '''Reads command line arguments'''
    parser = ArgumentParser(description='Generates .hh files from Ares .comp files')
    parser.add_argument('--indir', '-i', required=True, help='The input directory where .comp files are')
    parser.add_argument('--outdir', '-o', required=True, help='The output directory where .hh files will be')
    return parser.parse_args(args)


if __name__ == '__main__':
    args = read_args()
    if not os.path.isdir(args.indir):
        raise RuntimeError('Invalid input directory: ' + args.indir)

    gen_all_comp_headers(args.indir, args.outdir)
