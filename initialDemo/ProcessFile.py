import re
import os
datafolder = "./data/"

def processObjdump(case: str) -> None:
    folder = datafolder + case
    input = case + "-armv8m.objdump"
    output = case + "Objdump.txt"
    inPath = os.path.join(folder, input)
    outPath = os.path.join(folder, output)
    with open(outPath, 'w') as w:
        with open(inPath, 'r') as r:
            lines = r.readlines()
            n = len(lines)
            for i in range(n):
                line = lines[i]
                if re.match("^\S{8} \<\S*\>", line):
                    w.writelines(line.strip(":\n") + "\n")
                if i == n - 1 and re.match("^ *\S*:", line) and line[8] == ":":
                    start = re.search("^ *\S", line).span()[1]
                    w.writelines("0" * start + line[start : 8] + " <__endFunctionMap__>\n")

def processInst(case: str):
    folder = datafolder + case
    input = "instLog_" + case + "_ori.txt"
    output = "instLog_" + case + ".txt"
    inPath = os.path.join(folder, input)
    outPath = os.path.join(folder, output)
    with open(outPath, 'w') as w:
        with open(inPath, 'r') as r:
            for line in r.readlines():
                if re.search(" INST_COUNT=\S* ", line):
                    instCountSpan = re.search(" INST_COUNT=\S* ", line).span()
                    intsCount = line[instCountSpan[0] + 14: instCountSpan[1] - 1]
                    pcSpan = re.search(" PC=\S ", line).span()
                    pc = line[pcSpan[0] + 6: pcSpan[1] - 1]
                    w.writelines("0" * (8 - len(pc)) + pc + " " + intsCount + "\n")

processObjdump("quicksort")
processInst("quicksort")