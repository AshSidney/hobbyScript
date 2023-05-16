import subprocess

def getRslts():
    count = 32
    results = {}
    for i in range(count):
        rslt = subprocess.run('/Projects/hobbyScript/build/bin/CppScriptTest.exe --gtest_filter=CoreModulePerformanceFixture*Cache*',
                            capture_output=True)
        for ln in rslt.stdout.splitlines():
            parts = ln.decode().split()
            if len(parts) > 2 and parts[1] == 'OK':
                idx = parts[3].find('.')
                id = parts[3][idx + 1:]
                tm = int(parts[4][1:])
                if id not in results:
                    results[id] = []
                results[id].append(tm)
        print(i)
    for key, data in results.items():
        print(key, sorted(data))

getRslts()