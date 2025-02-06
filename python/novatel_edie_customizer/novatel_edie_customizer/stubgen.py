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
import json
import re
import textwrap
from typing import Union

import typer
from typing_extensions import Annotated

class StubGenerator:
    """Generator of type hint stub files for the novatel_edie package."""
    data_type_to_pytype = {
        'INT': 'int',
        'UNIT': 'int',
        'BOOL': 'bool',
        'CHAR': 'int',
        'UCHAR': 'int',
        'SHORT': 'int',
        'USHORT': 'int',
        'LONG': 'int',
        'ULONG': 'int',
        'LONGLONG': 'int',
        'ULONGLONG': 'int',
        'FLOAT': 'float',
        'DOUBLE': 'float',
        'HEXBYTE': 'int',
        'SATELLITEID': 'SatelliteId',
        'UNKNOWN': 'bytes'
    }
    def __init__(self, database: Union[str, dict]):
        """Initializes a StubGenerator.

        Args:
            database: A path to a message database json file or
                a dictionary representation of a database.
        """
        if isinstance(database, str):
            with open(database, 'r') as f:
                database = json.load(f)
        if not isinstance(database, dict):
            raise TypeError(
                'database must be a dict or a path to a JSON database file.')
        self.database = database
        self.database['enums_by_id'] = {enum['_id']: enum for enum in database['enums']}

    def write_stub_files(self, file_path: str):
        """Writes package stub files to the specified directory.

        Args:
            file_path: The directory to write the stub files to.
        """
        file_specs = {
            'enums.pyi': self.get_enum_stubs(),
            'messages.pyi': self.get_message_stubs()}

        if not os.path.exists(file_path):
            os.makedirs(file_path)
        for file_name, file_contents in file_specs.items():
            with open(os.path.join(file_path, file_name), 'w') as f:
                f.write(file_contents)

    def _convert_enum_def(self, enum_def) -> str:
        """Create a type hint string for an enum definition.

        Args:
            enum_def: A dictionary containing an enum definition.
        Returns:
            A string containing a type hint for the enum definition.
        """
        type_hint = f'class {enum_def["name"]}(Enum):\n'
        for enumerator in enum_def['enumerators']:
            name = enumerator['name']
            name = re.sub(r'[)(\-+./\\]', '_', name)
            if name[0].isdigit():
                name = f'_{name}'
            type_hint += f'    {name} = {enumerator["value"]}\n'
        return type_hint

    def get_enum_stubs(self) -> str:
        """Get a stub string for all enums in the database.

        Returns:
            A string containing type hint stubs for all enums in the database.
        """
        stub_str = ('from enum import Enum\n'
                    'from typing import Any\n\n')
        enums = self.database['enums']
        type_hints = [self._convert_enum_def(enum_def) for enum_def in enums]
        type_hints_str = '\n'.join(type_hints)
        stub_str += type_hints_str
        return stub_str

    def _get_field_pytype(self, field: dict, parent: str) -> str:
        """Get a type hint string for a field definition.

        Args:
            field: A dictionary containing a field definition.
            parent: The name of the parent class. May be used for naming
                subfield classes.
        Returns:
            A string containing a type hint for the field definition.
        """
        python_type = None
        if field['type'] == 'SIMPLE':
            python_type = self.data_type_to_pytype.get(field['dataType']['name'])
        if field['type'] == 'ENUM':
            enum_def = self.database['enums_by_id'].get(field['enumID'])
            python_type = enum_def['name'] if enum_def else 'Any'
        if field['type'] == 'STRING':
            python_type = 'str'
        if (field['type'] in {
                'FIXED_LENGTH_ARRAY',
                'VARIABLE_LENGTH_ARRAY'}):
            if field['conversionString'] == r'%s':
                python_type = 'str'
            else:
                python_type = f'list[{self.data_type_to_pytype.get(field["dataType"]["name"])}]'
        if field['type'] == 'FIELD_ARRAY':
            python_type = f'list[{parent}_{field["name"]}_Field]'
        if not python_type:
            python_type = 'Any'
        return python_type

    def _convert_field_array_def(self, field_array_def: dict, parent: str) -> str:
        """Convert a field array definition to a type hint string.

        Args:
            field_array_def: A dictionary containing a field array definition.
            parent: The name of the parent class. The name for the field array
                class will be based on this.
        Returns:
            A string containing type hint stubs for the field array definition.
        """
        subfield_hints = []

        # Create MessageBodyField type hint
        name = f'{parent}_{field_array_def["name"]}_Field'
        type_hint = f'class {name}(MessageBody):\n'
        for field in field_array_def['fields']:
            python_type = self._get_field_pytype(field, parent)
            type_hint +=  ('    @property\n'
                          f'    def {field["name"]}(self) -> {python_type}: ...\n\n')
            # Create hints for any subfields
            if field['type'] == 'FIELD_ARRAY':
                subfield_hints.append(self._convert_field_array_def(field, name))

        # Combine all hints
        hints = subfield_hints + [type_hint]
        hint_str = '\n'.join(hints)

        return hint_str

    def _convert_message_def(self, message_def: dict) -> str:
        """Create a type hint string for a message definition.

        Args:
            message_def: A dictionary containing a message definition.
        Returns:
            A string containing type hint stubs for the message definition.
        """
        subfield_hints = []

        # Create the Message type hint
        name = message_def['name']
        message_hint = textwrap.dedent(f"""\
            class {name}(Message):
                @property
                def header(self) -> Header: ...
                @property
                def body(self) -> {name}_Body: ...
        """)

        # Create the MessageBody type hint
        body_name = f'{message_def["name"]}_Body'
        body_hint = f'class {body_name}(MessageBody):\n'
        fields = message_def['fields'][message_def['latestMsgDefCrc']]
        if not fields:
            body_hint += '    pass\n\n'
        for field in fields:
            python_type = self._get_field_pytype(field, body_name)
            body_hint +=  '    @property\n'
            body_hint += f'    def {field["name"]}(self) -> {python_type}: ...\n\n'

            # Create hints for any subfields
            if field['type'] == 'FIELD_ARRAY':
                subfield_hints.append(self._convert_field_array_def(field, body_name))

        # Combine all hints
        hints = subfield_hints + [body_hint, message_hint]
        hint_str = '\n'.join(hints)

        return hint_str

    def get_message_stubs(self) -> str:
        """Get a stub string for all messages in the database.

        Returns:
            A string containing type hint stubs for all messages in the database.
        """
        stub_str = 'from typing import Any\n\n'
        stub_str += 'from novatel_edie import Header, Message, MessageBody, SatelliteId\n'
        stub_str += 'from novatel_edie.enums import *\n\n'

        messages = self.database['messages']
        type_hints = [self._convert_message_def(msg_def)
                      for msg_def in messages]
        type_hints_str = '\n'.join(type_hints)
        stub_str += type_hints_str

        return stub_str


def generate_stubs(
        database: Annotated[
            str,
            typer.Argument(help='A path to a database file.')
        ],
        output_dir: Annotated[
            str,
            typer.Argument(help='The directory to write stub files to.')
        ] = './stubs'
        ):
    """Generate type hint stub files for a provided database.

    Args:
        database: A path to a database file.
        output_dir: The directory to write stub files to.
    """
    StubGenerator(database).write_stub_files(output_dir)

if __name__ == '__main__':
    typer.run(generate_stubs)
