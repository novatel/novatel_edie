import os
import sys
import json
import argparse
import warnings

CPP_MSG_DEFS = {}
TAB_CHAR = '\t'

WEBSITE_LOGS = ['ALIGNBSLNENU', 'ALIGNBSLNXYZ', 'ALIGNDOP', 'ALMANAC', 'AUTHCODES', 'AVEPOS', 'BDSALMANAC',
'BDSBCNAV1EPHEMERIS', 'BDSBCNAV1RAWMESSAGE', 'BDSBCNAV2EPHEMERIS', 'BDSBCNAV2RAWMESSAGE', 'BDSCLOCK', 'BDSEPHEMERIS',
'BDSIONO', 'BDSRAWNAVSUBFRAME', 'BESTDATUMINFO', 'BESTGNSSDATUMINFO', 'BESTPOS', 'BESTSATS', 'BESTUTM', 'BESTVEL',
'BESTVELX', 'BESTXYZ', 'BSLNXYZ', 'CHANCONFIGLIST', 'CLOCKMODEL', 'CLOCKSTEERING', 'DATUMTRANSFORMATIONS',
'DUALANTENNAHEADING', 'ETHSTATUS', 'FILELIST', 'FILESTATUS', 'FILESYSTEMCAPACITY', 'FILESYSTEMSTATUS',
'FILETRANSFERSTATUS', 'GALALMANAC', 'GALCLOCK', 'GALCNAVRAWPAGE', 'GALFNAVEPHEMERIS', 'GALFNAVRAWALMANAC',
'GALFNAVRAWEPHEMERIS', 'GALFNAVRAWPAGE', 'GALINAVEPHEMERIS', 'GALINAVRAWALMANAC', 'GALINAVRAWEPHEMERIS',
'GALINAVRAWWORD', 'GALIONO', 'GEODETICDATUMS', 'GLMLA', 'GLOALMANAC', 'GLOCLOCK', 'GLOEPHEMERIS', 'GLORAWALM',
'GLORAWEPHEM', 'GLORAWFRAME', 'GLORAWSTRING', 'GPALM', 'GPGGA', 'GPGGALONG', 'GPGLL', 'GPGRS', 'GPGSA', 'GPGST',
'GPGSV', 'GPHDT', 'GPHDTDUALANTENNA', 'GPRMB', 'GPRMC', 'GPSCNAVRAWMESSAGE', 'GPSEPHEM', 'GPVTG', 'GPZDA',
'HEADING2', 'HEADINGRATE', 'HEADINGSATS', 'HWMONITOR', 'IONUTC', 'IPSTATS', 'IPSTATUS', 'ITBANDPASSBANK',
'ITDETECTSTATUS', 'ITFILTTABLE', 'ITPROGFILTBANK', 'ITPSDDETECT', 'ITPSDFINAL', 'J1939STATUS', 'LBANDBEAMTABLE',
'LBANDTRACKSTAT', 'LOGLIST', 'LUAFILELIST', 'LUAFILESYSTEMSTATUS', 'LUAOUTPUT', 'LUASTATUS', 'MARKPOS', 'MARK2POS',
'MARK3POS', 'MARK4POS', 'MARK1TIME', 'MARK2TIME', 'MARK3TIME', 'MARK4TIME', 'MASTERPOS', 'MATCHEDPOS',
'MATCHEDSATS', 'MATCHEDXYZ', 'MODELFEATURES', 'NAVICALMANAC', 'NAVICEPHEMERIS', 'NAVICIONO', 'NAVICRAWSUBFRAME',
'NAVICSYSCLOCK', 'NAVIGATE', 'OCEANIXINFO', 'OCEANIXSTATUS', 'PASSCOM', 'PASSAUX', 'PASSUSB', 'PASSETH1', 'PASSICOM',
'PASSNCOM', 'PASSTHROUGH', 'PDPDATUMINFO', 'PDPDOP', 'PDPDOP2', 'PDPPOS', 'PDPSATS', 'PDPVEL', 'PDPXYZ', 'PORTSTATS',
'PPPDATUMINFO', 'PPPDOP', 'PPPDOP2', 'PPPPOS', 'PPPSATS', 'PPPSEEDAPPLICATIONSTATUS', 'PPPSEEDSTORESTATUS',
'PROFILEINFO', 'PSRDATUMINFO', 'PSRDOP', 'PSRDOP2', 'PSRPOS', 'PSRSATS', 'PSRVEL', 'PSRXYZ', 'QZSSALMANAC',
'QZSSCNAVRAWMESSAGE', 'QZSSEPHEMERIS', 'QZSSIONUTC', 'QZSSRAWALMANAC', 'QZSSRAWCNAVMESSAGE', 'QZSSRAWEPHEM',
'QZSSRAWSUBFRAME', 'RADARSTATUS', 'RAIMSTATUS', 'RANGE', 'RANGECMP', 'RANGECMP2', 'RANGECMP3', 'RANGECMP4',
'RANGEGPSL1', 'RAWALM', 'RAWCNAVFRAME', 'RAWEPHEM', 'RAWGPSSUBFRAME', 'RAWGPSWORD', 'RAWSBASFRAME', 'RAWSBASFRAME2',
'REFSTATION', 'REFSTATIONINFO', 'ROVERPOS', 'RTKASSISTSTATUS', 'RTKDATUMINFO', 'RTKDOP', 'RTKDOP2', 'RTKPOS', 'RTKSATS',
'RTKVEL', 'RTKXYZ', 'RXCONFIG', 'RXSTATUS', 'RXSTATUSEVENT', 'SAFEMODESTATUS', 'SATEL4INFO', 'SATEL9INFO', 'SATELSTATUS',
'SATVIS2', 'SATXYZ2', 'SAVEDSURVEYPOSITIONS', 'SBAS0', 'SBAS1', 'SBAS10', 'SBAS12', 'SBAS17', 'SBAS18', 'SBAS2', 'SBAS24',
'SBAS25', 'SBAS26', 'SBAS27', 'SBAS3', 'SBAS4', 'SBAS5', 'SBAS6', 'SBAS7', 'SBAS9', 'SBASALMANAC', 'SKCALIBRATESTATUS',
'SOFTLOADSTATUS', 'SOURCETABLE', 'SPRINKLERDATA', 'SPRINKLERDATAH', 'TECTONICSCOMPENSATION', 'TERRASTARINFO',
'TERRASTARSTATUS', 'TILTDATA', 'TILTSTATUS', 'TIME', 'TIMESYNC', 'TRACKSTAT', 'TRANSFERPORTSTATUS', 'UPTIME',
'USERANTENNA', 'USERI2CRESPONSE', 'VALIDMODELS', 'VERIPOSINFO', 'VERIPOSSTATUS', 'VERSION', 'WIFIAPSETTINGS',
'WIFINETLIST', 'WIFISTATUS', 'BESTGNSSPOS', 'BESTGNSSVEL', 'CORRIMUDATA', 'CORRIMUDATAS', 'CORRIMUS', 'DELAYEDHEAVE',
'GIMBALLEDPVA', 'HEAVE', 'IMURATECORRIMUS', 'IMURATEPVA', 'IMURATEPVAS', 'INSATT', 'INSATTQS', 'INSATTS', 'INSATTX',
'INSCALSTATUS', 'INSCONFIG', 'INSDATUMINFO', 'INSPOS', 'INSPOSS', 'INSPOSX', 'INSPVA', 'INSPVACMP', 'INSPVAS',
'INSPVASDCMP', 'INSPVAX', 'INSSEEDSTATUS', 'INSSPD', 'INSSPDS', 'INSSTDEV', 'INSSTDEVS', 'INSUPDATESTATUS', 'INSVEL',
'INSVELS', 'INSVELUSER', 'INSVELX', 'MARK1PVA', 'MARK2PVA', 'MARK3PVA', 'MARK4PVA', 'PASHR', 'RAWDMI', 'RAWIMU', 'RAWIMUS',
'RAWIMUSX', 'RAWIMUX', 'RELINSPVA', 'SYNCHEAVE', 'SYNCRELINSPVA', 'TAGGEDMARK1PVA', 'TAGGEDMARK2PVA', 'TAGGEDMARK3PVA',
'TAGGEDMARK4PVA', 'TSS1', 'VARIABLELEVERARM']


NOVATEL_TO_CTYPES = {
    # c++ standard data type conversion
    'BOOL': 'bool',
    'BOOLCHAR': 'unsigned char',
    'CHAR': 'char',
    'UCHAR': 'uint8_t',
    'SHORT': 'int16_t',
    'USHORT': 'uint16_t',
    'INT': 'int32_t',
    'UINT': 'uint32_t',
    'LONG': 'int32_t',
    'ULONG': 'uint32_t',
    'ENUM': 'int32_t',
    'FLOAT': 'float',
    'DOUBLE': 'double',
    'LDOUBLE': 'long double',
    'LONGLONG': 'int64_t',
    'ULONGLONG': 'uint64_t',


    # NovAtel specific data type conversion
    'HEXBYTE': 'uint8_t',
    'SATELLITEID': 'uint32_t',
    'RTCMV3_CHAR8': 'int8_t',
    'RTCMV3_UINT8': 'uint8_t',
    'RTCMV3_INT16': 'int16_t'
}

RESERVED_CPP_NAMES = {
    'switch': 'sw',
    'operator': 'opr'
}


def gen_flat_structs(msg_database: dict, out_file: str = 'novatel_message_definitions.hpp', all_logs: bool = False):
    global CPP_MSG_DEFS
    if all_logs:
        print('All logs is TRUE')
    else:
        print('All logs is FALSE')
    for msg in msg_database['messages']:
        if not all_logs and msg['name'] not in WEBSITE_LOGS:
            continue

        gen_field_defs(msg['fields'][msg['latestMsgDefCrc']], msg['name'])

    with open(out_file, 'w') as fp:
        fp.write('#ifndef NOVATEL_MESSAGE_DEFINITIONS_HPP\n#define NOVATEL_MESSAGE_DEFINITIONS_HPP\n\n')
        fp.write('#ifdef PASSTHROUGH\n   #undef PASSTHROUGH // Fix name collision in wingdi.h (included by spdlog)\n#endif\n\n')
        fp.write('#include <cstdint>\n\n')
        fp.write('namespace novatel::edie::oem {\n\n')
        fp.write('#pragma pack(1)\n\n')
        for msg_name, msg_fields in CPP_MSG_DEFS.items():
            msg_def = f'struct {msg_name}\n{{\n'
            for field in msg_fields:
                msg_def += f'{TAB_CHAR}{field};\n'
            msg_def += '};\n\n'
            fp.write(msg_def)
        fp.write('\n#pragma pack()')
        fp.write('\n}')
        fp.write('\n\n#endif //NOVATEL_MESSAGE_DEFINITIONS_HPP\n')


def gen_field_defs(fields: dict, msg_name: str):
    global CPP_MSG_DEFS
    msg_fields = []

    for field in fields:
        if field['name'] in RESERVED_CPP_NAMES:
            field['name'] = RESERVED_CPP_NAMES[field['name']]
        if field['type'] == 'SIMPLE':
            msg_fields.append(f'{NOVATEL_TO_CTYPES[field["dataType"]["name"]]} {field["name"]}')
        elif field['type'] == 'ENUM':
            #  TODO: Generate a enums.hpp so ENUMS can be translated to a enum type
            msg_fields.append(f'{NOVATEL_TO_CTYPES["LONG"]} {field["name"]}')
        elif field['type'] == 'STRING':
            msg_fields.append(f'{NOVATEL_TO_CTYPES["UCHAR"]} {field["name"]}[{field["arrayLength"]}]')
        elif field['type'] == 'FIXED_LENGTH_ARRAY':
            msg_fields.append(f'{NOVATEL_TO_CTYPES[field["dataType"]["name"]]} {field["name"]}[{field["arrayLength"]}]')
        elif field['type'] == 'VARIABLE_LENGTH_ARRAY':
            msg_fields.append(f'{NOVATEL_TO_CTYPES["ULONG"]} {field["name"]}_arraylength')
            msg_fields.append(f'{NOVATEL_TO_CTYPES[field["dataType"]["name"]]} {field["name"]}[{field["arrayLength"]}]')
        elif field['type'] == 'FIELD_ARRAY':
            if not field["arrayLength"]:
                return
            struct_name = f'{msg_name}_{field["name"]}'
            msg_fields.append(f'{NOVATEL_TO_CTYPES["ULONG"]} {field["name"]}_arraylength')
            msg_fields.append(f'{struct_name} {field["name"]}[{field["arrayLength"]}]')
            gen_field_defs(field['fields'], struct_name)
        else:
            print(f'Unknown type: {field["type"]}')

    CPP_MSG_DEFS[msg_name] = msg_fields


def is_valid_file(parser, arg):
    if not os.path.exists(arg):
        parser.error(f'The file {arg} does not exist!')
    else:
        return open(arg, 'r')


def parse_args():
    p = argparse.ArgumentParser()
    p.add_argument('json_db', help='Path to the NovAtel JSON database', type=lambda x: is_valid_file(p, x))
    p.add_argument('-o', '--out_file', help='Output header file name. Defaults to "novatel_message_definitions.hpp"',
                   default="novatel_message_definitions.hpp")
    p.add_argument('-ts', '--tab_space', action='store_true', help='Use 4 spaces instead of a tab character',
                   default=False)
    p.add_argument('-al', '--all-logs', action='store_true', default=False,
                   help='Default is website logs only. Use this switch to generate structs for all logs')
    return p.parse_args()


if __name__ == '__main__':
    warnings.warn('This script is will only generate the newest revision of a message in the message definition json.')
    print()

    args = parse_args()
    if args.tab_space:
        TAB_CHAR = ' ' * 4

    msg_defs = json.load(args.json_db)
    gen_flat_structs(msg_defs, out_file=args.out_file, all_logs=args.all_logs)
    print(f'{args.out_file} generated')
    sys.exit()

