#!/usr/bin/env bash

converter_components=${1:-converter_components}
script_dir=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

if ! command -v "$converter_components" > /dev/null; then
  echo "$converter_components not found"
  exit 1
fi

failures=()
for FORMAT in "ASCII" "ABBREV_ASCII" "BINARY" "FLATTENED_BINARY" "JSON"; do
    "$converter_components" "$script_dir/../database/messages_public.json" "$script_dir/BESTUTMBIN.GPS" $FORMAT > /dev/null
    retval=$?
    if [ $retval -ne 0 ]; then
      echo "converter_components failed for $FORMAT failed with status $retval"
    fi

    if ! cmp -s "$script_dir/BESTUTMBIN.GPS.$FORMAT" "$script_dir/targets/BESTUTMBIN.GPS.$FORMAT"; then
        failures+=("OEM-$FORMAT")
    else
      rm "$script_dir/BESTUTMBIN.GPS.$FORMAT"
    fi
    
    if ! cmp -s "$script_dir/BESTUTMBIN.GPS.$FORMAT.UNKNOWN" "$script_dir/targets/BESTUTMBIN.GPS.$FORMAT.UNKNOWN"; then
        failures+=("OEM-$FORMAT.UNKNOWN")
    else
      rm "$script_dir/BESTUTMBIN.GPS.$FORMAT.UNKNOWN"
    fi
done

if [ ${#failures} -gt 0 ]; then
    echo "One or more regression tests failed! ${failures[*]}"
    exit 1
fi

