"""
Generates an HTML file documenting the JSON database using Bootstrap 5.

NOTE: The user needs to have the "json2html" module for this script to work. Run `pip install json2html` to install
the module.
"""
import argparse
import json
import xml.etree.ElementTree as ET
from pathlib import Path
from typing import TypedDict

from json2html import *

MESSAGE_KEYS = ["name", "description", "latestMsgDefCrc", "messageType", "fields"]
FIELDS_KEYS = ["name", "description", "dataType", "enumID"]
DATATYPE_KEYS = ["name", "description", "length"]


class Enum(TypedDict):
    _id: str
    enumerators: list
    name: str


Enums = list[Enum]


def filter_dictionary(dictionary: dict, keys: list) -> dict:
    """Filters a dictionary for the given keys.

    Args:
        dictionary: Dictionary to filter.
        keys: Keys to filter by.

    Returns:
        Dictionary with only the given keys and corresponding values.
        Value is None if the key isn't in the dictionary.
    """
    return {key: dictionary.get(key) for key in keys}


def generate_html_tables_from_json(json_data: dict) -> str:
    """Generates HTML tables from JSON data.

    Args:
        json_data: Dictionary of JSON data.

    Returns:
        String containing HTML tables.
    """
    filtered_json_messages = []

    for message in json_data["messages"]:
        filtered_message = filter_dictionary(message, MESSAGE_KEYS)
        filtered_fields = {}

        for crc, fields in filtered_message["fields"].items():
            filtered_crc_fields = []

            for field in fields:
                filtered_field = filter_dictionary(field, FIELDS_KEYS)
                filtered_field["dataType"] = filter_dictionary(
                    filtered_field["dataType"], DATATYPE_KEYS)

                filtered_crc_fields.append(filtered_field)
            filtered_fields[crc] = filtered_crc_fields
        filtered_message["fields"] = filtered_fields
        filtered_json_messages.append(filtered_message)

    return json2html.convert(json=filtered_json_messages,
                             table_attributes='class="table table-bordered table-hover"')


def format_html(html: str, enum_data: Enums) -> ET.ElementTree:
    """Formats the HTML output from json2html to look better by removing data and using Bootstrap.

    Args:
        html: String of HTML tables.
        enum_data: List of enums where each enum is a dictionary.

    Returns:
        ElementTree of the formatted HTML document.
    """
    root = ET.Element("html")
    head = ET.SubElement(root, "head")
    ET.SubElement(head, "link", attrib={
        "href": "https://cdn.jsdelivr.net/npm/bootstrap@5.2.2/dist/css/bootstrap.min.css",
        "rel": "stylesheet",
        "integrity": "sha384-Zenh87qX5JnK2Jl0vWa8Ck2rdkQ2Bzep5IDxbcnCeuOxjzrPF/et3URy9Bv1WTRi",
        "crossorigin": "anonymous"
    })
    ET.SubElement(head, "meta", attrib={
        "charset": "utf-8",
        "name": "viewport",
        "content": "width=device-width, initial-scale=1"
    })

    body = ET.SubElement(root, "body", attrib={"class": "m-3"})
    title = ET.SubElement(body, "h1")
    title.text = "Novatel Messages"

    html = html.replace("[Brief Description]", "")
    table = ET.fromstring(html)
    cmds_table = ET.Element("table", attrib={"class": "table table-bordered table-hover"})
    logs_table = ET.Element("table", attrib={"class": "table table-bordered table-hover"})
    table_header = table.find("thead")
    cmds_table.append(table_header)
    logs_table.append(table_header)

    table_body = table.find("tbody")
    for message in table_body.findall("tr"):
        message_data = parse_message(message)
        message_type = message_data["messageType"].text
        if message_type == "COMMAND":
            cmds_table.append(message)
        elif message_type == "LOG":
            logs_table.append(message)

    row = ET.SubElement(body, "div", attrib={"class": "row"})
    msg_type_col = ET.SubElement(row, "div", attrib={"class": "col-2"})
    msg_type_tablist = ET.SubElement(msg_type_col, "div", attrib={
        "class": "list-group sticky-top",
        "id": "list-tab",
        "role": "tablist"
    })

    create_tablist(msg_type_tablist, "commands", "COMMANDS", True)
    create_tablist(msg_type_tablist, "logs", "LOGS")
    create_tablist(msg_type_tablist, "enums", "ENUMS")

    msg_type_content_col = ET.SubElement(row, "div", attrib={"class": "col-10"})
    msg_type_content = ET.SubElement(msg_type_content_col, "div", attrib={
        "class": "tab-content",
        "id": "nav-tabContent"
    })

    cmds_table[:] = sorted(
        cmds_table.findall("tr"), key=lambda msg: parse_message(msg)["name"].text)
    logs_table[:] = sorted(
        logs_table.findall("tr"), key=lambda msg: parse_message(msg)["name"].text)
    enum_data[:] = sorted(enum_data, key=lambda enum: enum["name"])

    cmds_list_group = generate_msg_type_list_group("commands", cmds_table, active=True)
    msg_type_content.append(cmds_list_group)
    logs_list_group = generate_msg_type_list_group("logs", logs_table, active=False)
    msg_type_content.append(logs_list_group)
    enums_list_group = generate_enum_list_group(enum_data)
    msg_type_content.append(enums_list_group)

    enum_modals = generate_enum_modals(enum_data)
    body.append(enum_modals)

    ET.SubElement(body, "script", attrib={
        "src": "https://cdn.jsdelivr.net/npm/bootstrap@5.2.2/dist/js/bootstrap.bundle.min.js",
        "integrity": "sha384-OERcA2EqjJCMA+/3y+gxIOqMEjwtxJY7qPCqsdltbNJuaOe923+mo//f6V8Qbsw3",
        "crossorigin": "anonymous",
    })

    return ET.ElementTree(root)


def generate_msg_type_list_group(msg_type: str, msg_type_table: ET.Element, active: bool) -> ET.Element:
    """Generates the message type's list group HTML element.

    Args:
        msg_type: The message type (E.g. Commands or Logs).
        msg_type_table: The message type's data.
        active: True if that message type list group should be active by default.

    Returns:
        Element of the message type list group.
    """
    content_tab_pane = ET.Element("div", attrib={
        "class": f"tab-pane fade {'show active' if active else ''}",
        "id": f"list-{msg_type}",
        "role": "tabpanel"
    })

    content_row = ET.SubElement(content_tab_pane, "div", attrib={"class": "row"})
    content_tablist_col = ET.SubElement(content_row, "div", attrib={"class": "col-3"})
    content_tablist = ET.SubElement(content_tablist_col, "div", attrib={
        "class": "list-group",
        "id": f"{msg_type}-content-tablist",
        "role": "tablist"
    })

    content_col = ET.SubElement(content_row, "div", attrib={"class": "col-8 sticky-top h-100"})
    content = ET.SubElement(content_col, "div", attrib={
        "class": "tab-content",
        "id": f"nav-{msg_type}-tab-content"
    })

    for msg in msg_type_table.findall("tr"):
        msg_data = parse_message(msg)
        msg_name = msg_data["name"].text
        create_tablist(content_tablist, msg_name, msg_name)

        msg_content = ET.SubElement(content, "div", attrib={
            "class": "tab-pane fade",
            "id": f"list-{msg_name}",
            "role": "tabpanel",
            "aria-labelledby": f"list-{msg_name}"
        })
        msg_content.text = msg_data["description"].text
        msg_content.append(format_message(msg))

    return content_tab_pane


def generate_enum_list_group(enum_data: Enums) -> ET.Element:
    """Generates the enum list group HTML element.

    Args:
        enum_data: A list of enums.

    Returns:
        Element of the enum list group.
    """
    content_tab_pane = ET.Element("div", attrib={
        "class": f"tab-pane fade",
        "id": f"list-enums",
        "role": "tabpanel"
    })

    content_row = ET.SubElement(content_tab_pane, "div", attrib={"class": "row"})
    content_tablist_col = ET.SubElement(content_row, "div", attrib={"class": "col-3"})
    content_tablist = ET.SubElement(content_tablist_col, "div", attrib={
        "class": "list-group",
        "id": f"enums-content-tablist",
        "role": "tablist"
    })

    content_col = ET.SubElement(content_row, "div", attrib={"class": "col-8 sticky-top h-100"})
    content = ET.SubElement(content_col, "div", attrib={
        "class": "tab-content overflow-auto",
        "id": f"nav-enums-tab-content",
        "style": "max-height: 75vh;"
    })

    for enum in enum_data:
        enum_name = enum["name"]
        enum_label = f"{enum_name}-enum"
        create_tablist(content_tablist, enum_label, enum_name)

        enum_content = ET.SubElement(content, "div", attrib={
            "class": "tab-pane fade",
            "id": f"list-{enum_label}",
            "role": "tabpanel",
            "aria-labelledby": f"list-{enum_label}"
        })
        enum_content.append(generate_enum_table(enum))

    return content_tab_pane


def format_message(msg: ET.Element) -> ET.Element:
    """Formats the message into a Bootstrap accordion element.

    Args:
        msg: Element of the message.

    Returns:
        Element of the message accordion.
    """
    parsed_msg = parse_message(msg)
    latest_msg_def_crc = parsed_msg["latestMsgDefCrc"].text
    msg_versions = sorted(parsed_msg["fields"].find("table"), key=lambda x: int(x.find("th").text),
                          reverse=True)

    accordion = ET.Element("div", attrib={
        "class": "accordion",
        "id": f"{parsed_msg['name'].text}-accordion"
    })

    for version in msg_versions:
        msg_crc = version.find("th").text
        is_latest_version = msg_crc == latest_msg_def_crc

        accordion_item = ET.SubElement(accordion, "div", attrib={"class": "accordion-item"})
        accordion_header = ET.SubElement(accordion_item, "h2", attrib={
            "class": "accordion-header",
            "id": f"{msg_crc}"
        })
        accordion_button = ET.SubElement(accordion_header, "button", attrib={
            "class": f"accordion-button collapsed {'fw-bold' if is_latest_version else ''}",
            "type": "button",
            "data-bs-toggle": "collapse",
            "data-bs-target": f"#panel-{msg_crc}",
            "aria-expanded": "false",
            "aria-controls": f"panel-{msg_crc}",
        })
        accordion_button.text = f"{msg_crc} (latest)" if is_latest_version else msg_crc
        accordion_collapse = ET.SubElement(accordion_item, "div", attrib={
            "class": f"accordion-collapse collapse {'show' if is_latest_version else ''}",
            "id": f"panel-{msg_crc}",
            "aria-labelledby": f"panel-{msg_crc}",
            "data-bs-parent": f"#{parsed_msg['name'].text}-accordion"
        })
        accordion_body = ET.SubElement(accordion_collapse, "div", attrib={
            "class": "accordion-body overflow-auto",
            "style": "max-height: 75vh;"
        })

        version_fields = version.find("td")
        if len(version_fields) > 0:
            version_fields = version_fields[0]

            for field in version_fields.find("tbody"):
                field_cols = field.findall("td")
                data_type_col = field_cols[2]
                enum_id_col = field_cols[3]
                formatted_data_type_col = format_data_type(data_type_col, enum_id_col)
                field.insert(2, formatted_data_type_col)
                field.remove(data_type_col)
                field.remove(enum_id_col)

            accordion_body.append(version_fields)

        # Make the first table row blue and remove the "enumID" column
        for table_head in accordion_body.iter("thead"):
            table_head.set("class", "table-primary")
            table_heads = table_head.find("tr").findall("th")
            for table_head_col in table_heads:
                if table_head_col.text == "enumID":
                    table_head.find("tr").remove(table_head_col)
                else:
                    table_head_col.text = table_head_col.text.title()

    return accordion


def format_data_type(data_type_col: ET.Element, enum_id_col: ET.Element) -> ET.Element:
    """Formats the data type column from the HTML table to simple text.

    Args:
        data_type_col: Element of the data type column.
        enum_id_col: Element of the enum ID column.

    Returns:
        Element of the simplified data type.
    """
    data_type_element = ET.Element("td")

    data_type_fields = data_type_col[0].findall("tr")
    data_type = {data_type_field.find("th").text: data_type_field.find("td").text for
                 data_type_field in data_type_fields}

    data_type_length = int(data_type["length"])
    unit = "bytes" if data_type_length > 1 else "byte"

    # If there's a description, use the <p> HTML element to add a newline before the description
    if data_type["description"]:
        data_type_cell = ET.SubElement(data_type_element, "p")
        description = ET.SubElement(data_type_element, "span")
        description.text = data_type["description"]

        if data_type["name"] == "ENUM":
            enum_link = ET.SubElement(data_type_cell, "a", attrib={
                "href": f"#_{enum_id_col.text}", "data-bs-toggle": "modal"})
            enum_link.text = f"{data_type['name']} ({data_type_length} {unit})"
        else:
            data_type_cell.text = f"{data_type['name']} ({data_type_length} {unit})"
    else:
        if data_type["name"] == "ENUM":
            data_type_cell = ET.SubElement(data_type_element, "a", attrib={
                "href": f"#_{enum_id_col.text}", "data-bs-toggle": "modal"})
        else:
            data_type_cell = ET.SubElement(data_type_element, "span")
        data_type_cell.text = f"{data_type['name']} ({data_type_length} {unit})"

    return data_type_element


def parse_message(msg: ET.Element) -> dict:
    """Parses a message for data in specific columns/keys.

    Args:
        msg: Element of the message.

    Returns:
        Dict mapping column name (string) to column value (ElementTree Element).
    """
    msg_cols = msg.findall("td")
    return dict(zip(MESSAGE_KEYS, msg_cols))


def generate_enum_modals(enum_data: Enums) -> ET.Element:
    """
    Generates the modals for when an enum is clicked. All enum modals are generated but only one is shown when clicked.

    Args:
        enum_data: List of enums where each enum is a dictionary.

    Returns:
        Element of the enum modals.
    """
    enum_modals_element = ET.Element("div", attrib={"class": "all-enum-modals"})

    for enum in enum_data:
        # Prefix ID with underscore because CSS doesn't allow for selectors to start with a number
        enum_id = f"_{enum['_id']}"
        enum_enumerators = enum["enumerators"]
        enum_name = enum["name"]

        enum_modal = ET.SubElement(enum_modals_element, "div", attrib={
            "class": "modal fade",
            "id": enum_id,
            "aria-hidden": "true",
            "aria-labelledby": enum_id,
            "tabindex": "-1"
        })
        enum_modal_dialog = ET.SubElement(enum_modal, "div", attrib={
            "class": "modal-xl modal-dialog modal-dialog-centered"
        })
        enum_modal_content = ET.SubElement(enum_modal_dialog, "div", attrib={"class": "modal-content"})
        enum_modal_header = ET.SubElement(enum_modal_content, "div", attrib={"class": "modal-header"})
        enum_modal_title = ET.SubElement(enum_modal_header, "h1", attrib={"class": "modal-title fs-5"})
        enum_modal_title.text = enum_name
        ET.SubElement(enum_modal_header, "button", attrib={"type": "button", "class": "btn-close",
                                                           "data-bs-dismiss": "modal", "aria-label": "Close"})

        enum_modal_body = ET.SubElement(enum_modal_content, "div", attrib={"class": "modal-body"})
        enum_modal_body.append(generate_enum_table(enum))

    return enum_modals_element


def generate_enum_table(enum_data: Enum) -> ET.Element:
    """
    Generates an HTML table representing an enum.

    Args:
        enum_data: List of enums where each enum is a dictionary.

    Returns:
        Element of the enum table.
    """
    enum_table = ET.Element("table", attrib={"class": "table"})
    enum_table_head = ET.SubElement(enum_table, "thead", attrib={"class": "table-primary"})
    enum_table_head_row = ET.SubElement(enum_table_head, "tr", attrib={"class": "sticky-top"})
    enum_table_headers = ["value", "name", "description"]
    for header in enum_table_headers:
        table_header = ET.SubElement(enum_table_head_row, "th", attrib={"scope": "col"})
        table_header.text = header.title()

    enum_table_body = ET.SubElement(enum_table, "tbody")
    for enumerators in enum_data["enumerators"]:
        enum_table_row = ET.SubElement(enum_table_body, "tr")
        for header in enum_table_headers:
            enum_table_cell = ET.SubElement(enum_table_row, "td")
            enum_table_cell.text = str(enumerators[header]) if enumerators[header] is not None else ""

    return enum_table


def create_tablist(parent: ET.Element, label: str, tab_text: str, active: bool = False) -> ET.Element:
    """
    Creates a Bootstrap tablist.

    Args:
        parent: Parent element to attach the tablist.
        label: The internal CSS label to give the tablist.
        tab_text: The text to display on the tablist's tab.
        active: If active then highlight the tab by default.

    Returns:
        Element of the tablist.
    """
    tablist = ET.SubElement(parent, "a", attrib={
        "class": f"list-group-item list-group-item-action {'active' if active else ''}",
        "id": f"list-{label}-list",
        "data-bs-toggle": "list",
        "href": f"#list-{label}",
        "role": "tab",
        "aria-controls": f"list-{label}",
        "aria-selected": f"{'true' if active else 'false'}"
    })
    tablist.text = tab_text

    return tablist


def save_html(data: ET.ElementTree, output_path: Path) -> Path:
    """Saves the HTML data to an HTML file.

    Args:
        data: HTML data.
        output_path: Path to save the HTML file.

    Returns:
        Path of where the HTML file was saved.
    """
    output_path = output_path.with_suffix(".html")
    data.write(output_path, encoding="us-ascii", method="html")
    return output_path


def parse_args() -> argparse.Namespace:
    """Parses program arguments.

    Returns:
        Results from argument parsing.
    """
    parser = argparse.ArgumentParser()
    parser.add_argument('json_db', help='Path to the NovAtel JSON database.')
    parser.add_argument('-o', '--out_file',
                        help='Output HTML file name. Defaults to the JSON file name')
    return parser.parse_args()


def main():
    """Main function to generate HTML documentation from the JSON database.
    """
    args = parse_args()
    json_database_path = Path(args.json_db)
    if not json_database_path.exists():
        raise FileNotFoundError(f"Database file doesn't exist: {json_database_path}")

    json_database = json.load(json_database_path.open())
    html_database = generate_html_tables_from_json(json_database)
    formatted_html_database = format_html(html_database, json_database["enums"])
    output_filename = args.out_file if args.out_file else json_database_path.stem
    html_path = save_html(formatted_html_database, Path(json_database_path.parent, output_filename))
    print(f"Generated the file at: {html_path}")


if __name__ == "__main__":
    main()
