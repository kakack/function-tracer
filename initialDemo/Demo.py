import matplotlib
import os
import time
import json

case = "quicksort"

functionMap = dict()
instructions = list()
dataFolder = "./data/"
folder = dataFolder + case

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
    objdumpPath = os.path.join(folder, case + "Objdump.txt")
    with open(objdumpPath, 'r') as f:
        lines = f.readlines()
        prev = ""
        for line in lines:
            content = line.split(' ')
            if prev:
                functionMap[prev].setAddrf(int(prev, 16), int(content[0], 16))
            if content[1].strip() != "<__endFunctionMap__>":
                functionMap[content[0]] = FuncAddr(content[1].strip())
                prev = content[0]
    instPath = os.path.join(folder, "instLog_" + case + ".txt")
    with open(instPath, 'r') as i:
        lines = i.readlines()
        for line in lines:
            instructions.append(line.strip().split())
            
if __name__ == "__main__":
    start = time.time()
    result = list()
    initMap()
    mapInitTime = time.time()
    
    print("Init  Time: " + str(mapInitTime - start))
    
    functionStack = list()
    
    # # check init
    # print("===============Function Map================")
    # for fm in functionMap:
    #     print("funcName: " + str(fm) + " addr: " + str(functionMap[fm]))
    # print("===============Instruction==================")
    # for inst in instructions:
    #     print(inst)
        
    for inst, ic in instructions:
        if functionMap.__contains__(inst):
            functionStack.append(functionMap[inst])
            result.append(FuncOp(functionMap[inst].name, "Call", ic))
        else:
            instAddr = int(inst, 16)
            while functionStack and not functionStack[-1].lowAddr < instAddr < functionStack[-1].highAddr:
                result.append(FuncOp(functionStack[-1].name, "Return", ic))
                functionStack.pop()
    
    end = time.time()
    print("Query Time: " + str(end - mapInitTime))
    print("Total Time: " + str(end - start))
    
    # # print mode
    # for item in result:
    #     print("function name: " + item.name + " Op:" + item.op + " ic: " + item.ic)
        
    # # txt file out mode
    # output = os.path.join(folder, "FuncTracer_" + case + ".txt")
    # with open(output, 'w') as f:
    #     for item in result:
    #         f.write(item.ic +  ' ' + item.name + ' ' + item.op + "\n")
            
    # json file out mode
    class FuncOpEncoder(json.JSONEncoder):
        def default(self, o: Any) -> Any:
            return o.__dict__
    
    jsonFilePath = os.path.join(folder, "FuncTracer_" + case + ".json")
    with open(jsonFilePath, 'w') as jf:
        resJson = json.dump(result, jf, cls=FuncOpEncoder, indent=4)