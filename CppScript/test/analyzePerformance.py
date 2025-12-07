import subprocess
import sys
import random

def getRslts(count, indexed, filters):
    results = {}
    for i in range(count):
        print(i)
        random.shuffle(filters)
        for filter in filters:
            print(filter)
            rslt = subprocess.run('build/RelWithDebInfo/CppScriptTest.exe --gtest_filter=' + filter,
                capture_output=True)
            for ln in rslt.stdout.splitlines():
                parts = ln.decode().split()
                if len(parts) > 2 and parts[1] == 'OK':
                    idx = parts[3].find('.')
                    id = parts[3][idx + 1:]
                    if indexed:
                        idx = id.find('/')
                        if idx > 0 and idx < len(id):
                            idx = int(id[idx + 1:])
                        if idx not in results:
                            results[idx] = {}
                        res = results[idx]
                    else:
                        res = results
                    tm = int(parts[4][1:])
                    if id not in res:
                        res[id] = []
                    res[id].append(tm)
    for key, data in results.items():
        if indexed:
            for idx, dt in data.items():
                print(idx, sorted(dt))
        else:
            print(key, sorted(data))

if len(sys.argv) > 1:
    if sys.argv[1] == 'int':
        getRslts(8, True, ['*FibonacciInstances*Variant*', '*FibonacciInstances*Custom*', '*FibonacciInstances*Virtual*', '*FibonacciInstances*Lambda*',
            '*FibonacciInstances*OldNoCache*', '*FibonacciInstances*OldCache*'])
    elif sys.argv[1] == 'stdint':
        getRslts(64, False, ['CoreModulePerformanceFixture.Fibonacci*', 'CoreOperationPerformanceStdIntFixture*'])