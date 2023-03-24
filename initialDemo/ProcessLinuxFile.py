import re

infile = "./data/linux/vmlinux_add.objdump"
outfile = "./data/linux/vmlinux_funcList.txt"
instFull = "./data/linux/cluster0.cpu0.tmarmac"
instOnly = "./data/linux/instLog.txt"

def processLinuxObjdump():
    with open(outfile, 'w') as w:
        with open(infile, 'r') as r:
            for line in r.readlines():
                if re.match("\S*DW_TAG_subprogram", line) and re.match(".*DW_AT_low_pc<\S*>", line) and re.match(".*DW_AT_name", line):
                    funcNamespan = re.search("DW_AT_name<\S*>", line).span()
                    funcName = line[funcNamespan[0] +11: funcNamespan[1] - 1]
                    lowAddrSpan = re.search("DW_AT_low_pc<\S*>", line).span()
                    lowAddr = line[lowAddrSpan[0] + 13: lowAddrSpan[1] - 1]
                    highAddrSpan = re.search("<highpc: \S*>", line).span()
                    highAddr = line[highAddrSpan[0] + 9: highAddrSpan[1] - 2]
                    w.writelines(funcName + " " + lowAddr + " " + highAddr + "\n")

def processLinuxInst():
    with open(instOnly, "w") as w:
        with open(instFull, 'r') as r:
            while True:
                line = r.readline()
                if not line:
                    break
                elif re.match(".* IT ", line):
                    content = line.split(' ')
                    if content[5][:4] == "ffff":
                        w.writelines(content[4].strip('(').strip(')') + " " + content[5].split(':')[0] + "\n")
                        

processLinuxInst()
processLinuxObjdump()        