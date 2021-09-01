import subprocess
import re

def getTestData():
    data = {}
    rslt = subprocess.run(['../x64/release/CppScriptTest.exe', '--gtest_filter=*Fibonacci*'], capture_output=True)
    for ln in rslt.stdout.splitlines():
        mtch = re.match('.*OperationsFixture\.(.*?) \(([0-9]+) .*', str(ln))
        if mtch is not None:
            data[mtch.group(1)] = int(mtch.group(2))
    return data

def collectTestData(cnt):
    data = {}
    while cnt > 0:
        nextSet = getTestData()
        for nm in nextSet.keys():
            if nm not in data:
                data[nm] = []
            data[nm].append(nextSet[nm])
        cnt -= 1
        print('count', cnt)
    return data

def calcAvg(data):
    sum = 0
    for val in data:
        sum += val
    return sum / len(data)

def calcAvgs(data):
    data.sort()
    return (calcAvg(data), calcAvg(data[:int(len(data)/2)]))

data = collectTestData(64)
for item in data.items():
    print(item[0], calcAvgs(item[1]))