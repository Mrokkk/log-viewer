#!/usr/bin/env python3

import re
import sys

deps_regex = r'^.*_deps/([a-zA-Z0-9]+)-build/(.*)'

def main():
    start_timestamp = int(sys.argv[2]) * 1000000000 if len(sys.argv) > 2 else 0
    with open(sys.argv[1], 'r') as f:
        data = []
        header = f.readline()

        time_column_width = 0
        component_column_width = 0

        while True:
            line = f.readline()

            if not line:
                break

            words = line.split()
            start = int(words[0])
            end = int(words[1])
            unit = words[3]

            if '.cmake' in unit or 'build.ninja' in unit:
                continue

            if int(words[2]) < start_timestamp:
                continue

            if '_deps' in unit:
                result = re.match(deps_regex, unit)
                if result:
                    unit = re.sub(deps_regex, r'\2', unit)
                    component = result.group(1)
                else:
                    component = 'unknown'
            else:
                component = 'log-viewer'
                unit = unit.replace('.o', '')
                if 'test.dir' in unit:
                    unit = unit.replace('test/CMakeFiles/test.dir/', 'test/')
                else:
                    unit = unit.replace('CMakeFiles/log-viewer.dir/', '')

            component_column_width = max(component_column_width, len(component))

            time_column_width = max(time_column_width, len(str(end - start)))

            data.append((unit, component, end - start))

        for entry in sorted(data, key=lambda entry: entry[2]):
            print(f'{str(entry[2]).rjust(time_column_width)} ms   [{entry[1].rjust(component_column_width)}] {entry[0]}')

if __name__ == '__main__':
    main()
