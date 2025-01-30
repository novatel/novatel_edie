"""
MIT

Copyright (c) 2023 NovAtel Inc.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

A module concerning the generation of type hint stub files for the novatel_edie package.
"""
import os

import nanobind

import nanobind.stubgen
import novatel_edie as ne
from novatel_edie import MessageDatabase, JsonDbReader, DATA_TYPE, MessageDefinition

class StubGenerator:
    """Generator of type hint stub files for the novatel_edie package."""
    data_type_to_pytype = {
        DATA_TYPE.INT: 'int',
        DATA_TYPE.UINT: 'int',
        DATA_TYPE.BOOL: 'bool',
        DATA_TYPE.CHAR: 'int',
        DATA_TYPE.UCHAR: 'int',
        DATA_TYPE.SHORT: 'int',
        DATA_TYPE.USHORT: 'int',
        DATA_TYPE.LONG: 'int',
        DATA_TYPE.ULONG: 'int',
        DATA_TYPE.LONGLONG: 'int',
        DATA_TYPE.ULONGLONG: 'int',
        DATA_TYPE.FLOAT: 'float',
        DATA_TYPE.DOUBLE: 'float',
        DATA_TYPE.HEXBYTE: 'int',
        DATA_TYPE.SATELLITEID: 'SatelliteId',
        DATA_TYPE.UNKNOWN: 'bytes'
    }
    def __init__(self, database: str | MessageDatabase):
        if isinstance(database, str):
            database = JsonDbReader.load_file(database)
        if not isinstance(database, MessageDatabase):
            raise TypeError(
                'database must be a MessageDatabase object or a path to a JSON database file.')
        self.database = database


    def write_stub_files(self, file_path: str):
        """Writes package stub files to the specified directory."""

        file_specs = {'__init__.pyi': self.get_core_stubs(),
                      'enums.pyi': self.get_enum_stubs(),
                      'messages.pyi': self.get_message_stubs()}

        if not os.path.exists(file_path):
            os.makedirs(file_path)
        for file_name, file_contents in file_specs.items():
            with open(os.path.join(file_path, file_name), 'w') as f:
                f.write(file_contents)

    def get_core_stubs(self) -> str:
        stub_gen = nanobind.stubgen.StubGen(ne.bindings)
        stub_gen.put(ne.bindings)
        stub_str = stub_gen.get()
        return stub_str

    def _convert_enum_def(self, enum_def) -> str:
        """Create a type hint string for an enum definition."""
        type_hint = f'class {enum_def.__name__}(Enum):\n'
        for enumeration in enum_def:
            type_hint += f'    {enumeration.name} = {enumeration.value}\n'
        return type_hint

    def get_enum_stubs(self) -> str:
        """Get a stub string for all enums in the database."""
        stub_str = ('from enum import Enum\n'
                    'from typing import Any\n\n')
        enums = list(self.database.enums.values())
        type_hints = [self._convert_enum_def(enum_def) for enum_def in enums]
        type_hints_str = '\n'.join(type_hints)
        stub_str += type_hints_str
        return stub_str

    def _get_field_pytype(self, field, parent: str) -> str:
        """Get a type hint string for a base level field."""
        python_type = None
        if field.type == ne.FIELD_TYPE.SIMPLE:
            python_type = self.data_type_to_pytype.get(field.data_type.name)
        if field.type == ne.FIELD_TYPE.ENUM:
            python_type = field.enum_def.name if field.enum_def else 'Enum'
        if field.type == ne.FIELD_TYPE.STRING:
            python_type = 'str'
        if (field.type in {
                ne.FIELD_TYPE.FIXED_LENGTH_ARRAY,
                ne.FIELD_TYPE.VARIABLE_LENGTH_ARRAY}):
            if field.conversion == r'%s':
                python_type = 'str'
            else:
                python_type = f'list[{self.data_type_to_pytype.get(field.data_type.name)}]'
        if field.type == ne.FIELD_TYPE.FIELD_ARRAY:
            python_type = f'list[{parent}_{field.name}_Field]'
        if not python_type:
            python_type = 'Any'
        return python_type

    def _convert_field_array_def(self, field_array_def, parent: str) -> str:
        """Convert a field array definition to a type hint string."""
        subfield_hints = []

        # Create MessageBodyField type hint
        name = f'{parent}_{field_array_def.name}_Field'
        type_hint = f'class {name}(MessageBody):\n'
        for field in field_array_def.fields:
            python_type = self._get_field_pytype(field, parent)
            type_hint +=  ('    @property\n'
                          f'    def {field.name}(self) -> {python_type}: ...\n\n')
            # Create hints for any subfields
            if field.type == ne.FIELD_TYPE.FIELD_ARRAY:
                subfield_hints.append(self._convert_field_array_def(field, name))

        # Combine all hints
        hints = subfield_hints + [type_hint]
        hint_str = '\n'.join(hints)

        return hint_str

    def _convert_message_def(self, message_def: MessageDefinition) -> str:
        """Create a type hint string for a message definition."""
        subfield_hints = []

        # Create the Message type hint
        message_hint = (f'class {message_def.name}(Message):\n'
                         '    @property\n'
                         '    def header(self) -> Header: ...\n\n'
                         '    @property\n'
                        f'    def body(self) -> {message_def.name}_Body: ...\n\n')

        # Create the MessageBody type hint
        body_name = f'{message_def.name}_Body'
        body_hint = f'class {body_name}(MessageBody):\n'
        for field in message_def.fields[message_def.latest_message_crc]:
            python_type = self._get_field_pytype(field, body_name)
            body_hint +=  '    @property\n'
            body_hint += f'    def {field.name}(self) -> {python_type}: ...\n\n'

            # Create hints for any subfields
            if field.type == ne.FIELD_TYPE.FIELD_ARRAY:
                subfield_hints.append(self._convert_field_array_def(field, body_name))

        # Combine all hints
        hints = subfield_hints + [body_hint, message_hint]
        hint_str = '\n'.join(hints)

        return hint_str

    def get_message_stubs(self) -> str:
        """Get a stub string for all messages in the database."""
        stub_str = 'from typing import Any\n\n'
        stub_str += 'from novatel_edie import Header, Message, MessageBody, SatelliteId\n'
        stub_str += 'from novatel_edie.enums import *\n\n'

        message_list = [key for key in self.database.messages
                        if not key.endswith('_Body')
                        and not key.endswith('_Field')
                        and key != 'UNKNOWN']
        message_defs = [self.database.get_msg_def(msg)
                        for msg in message_list]
        type_hints = [self._convert_message_def(msg_def)
                      for msg_def in message_defs]
        type_hints_str = '\n'.join(type_hints)
        stub_str += type_hints_str

        return stub_str

if __name__ == '__main__':
    StubGenerator(ne.JsonDbReader.load_file('database/database.json')).write_stub_files('stubs')
