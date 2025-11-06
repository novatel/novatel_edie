import argparse
from pathlib import Path

def main():
    parser = argparse.ArgumentParser(description='Process a file path')
    parser.add_argument('filepath', help='Path to the file to process')

    args = parser.parse_args()

    # Convert to pathlib Path
    filepath = Path(args.filepath)
    oem_stub_path = filepath / 'python_oem.pyi'

    print(f'PATH: {oem_stub_path}')
    with open(oem_stub_path, 'r') as file:
        lines = file.readlines()

    # Process lines to replace exact "import py_common" lines
    processed_lines = []
    for line in lines:
        if line.strip() == 'import python_common':
            processed_lines.append('import novatel_edie.python_common as python_common\n')
        else:
            processed_lines.append(line)

    # Write back to file
    with open(oem_stub_path, 'w') as file:
        file.writelines(processed_lines)



if __name__ == '__main__':
    main()
