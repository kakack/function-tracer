import matplotlib
import os
import time
import json

instFile = "./data/linux/instLog.txt"
funcList = "./data/linux/vmlinux_funcList.txt"
functionMap = dict()

class FuncAddr:
    def __init__(self, funcName: str) -> None:
        self.name = funcName
        
    def setAddr(self, lowAddr: int, highAddr: int) -> None:
        self.lowAddr = lowAddr
        self.highAddr = highAddr
        
class FuncOp:
    def __init__(self, funcName: str, funcOp: str, ic: str) -> None:
        self.name = funcName
        self.op = funcOp
        self.ic = ic
        
def initMap():
    with open(funcList, 'r') as f:
        lines = f.readlines()
        for line in lines:
            content = line.split(' ')
            funcAddr = FuncAddr(content[0])
            lowAddr, highAddr = int(content[1], 16), int(content[2], 16)
            funcAddr.setAddr(lowAddr, highAddr)
            functionMap[lowAddr] = funcAddr
            
if __name__ == "__main__":
    start = time.time()
    result = list()
    initMap()
    mapInitTime = time.time()
    
    print("Map init time: " + str(mapInitTime - start))
    functionStack = list()
    
    with open(instFile, 'r') as insts:
        while True:
            instline = insts.readline()
            if not instline:
                break
            instContent = instline.strip().split()
            if len(instContent) == 2:
                ic = int(instContent[0])
                inst = int(instContent[1], 16)
                if functionMap.__contains__(inst):
                    functionStack.append(functionMap[inst])
                    result.append(FuncOp(functionMap[inst].name, "Call", ic))
                else:
                    while functionStack and not functionStack[-1].lowAddr < inst < functionStack[-1].highAddr:
                        result.append(FuncOp(functionMap[inst].name, "Return", ic))
                        functionStack.pop()
            else:
                print(instline)
                
    end = time.time()
    print("Query time:" + str(end - mapInitTime))
    print("Total time: " + str(end - start))
    
    print("Result size: " + str(len(result)))
    
    class FuncOpEncoder(json.JSONEncoder):
        def default(self, o):
            return o.__dict__
        
    jsonFilePath = "./data/linux/FunTracer_linux.json"
    with open(jsonFilePath, 'w') as jf:
        resJson =  json.dump(result, jf, cls=FuncOpEncoder, indent=4)
        