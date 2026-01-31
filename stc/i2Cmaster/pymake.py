import os
import sys
import pdb

#for file: PATH:dir+filename, DIR:dir, FILENAME:filename
#for dir : PATH:dir, DIR:dir

class DIR:
    def __init__(self, path):
        self.dir = []
        self.file = []
        self.path = path
        self.refresh()
    
    def refresh(self):
        self.dir = []
        self.file = []
        if os.path.exists(self.path):
            for i in os.listdir(self.path):
                if os.path.isdir(os.path.join(self.path, i)):
                    self.dir.append(DIR(os.path.join(self.path, i)))
                if os.path.isfile(os.path.join(self.path, i)):
                    self.file.append(FILE(os.path.join(self.path, i)))
                
class FILE:
    def __init__(self, path):
        self.path = path
        self.dir = os.path.dirname(self.path)
        self.filename = os.path.basename(self.path)

def getAllCfiles(rootDir):
    Cfileslist = []
    for i in rootDir.file:
        if i.filename.endswith(".c"):
            Cfileslist.append(i)
    
    for i in rootDir.dir:
        Cfileslist.extend(getAllCfiles(i))
    
    return Cfileslist

def makeCmd(*args):
    def deList(inputList):
        objects = []
        for i in inputList:
            if type(i) == list:
                objects.extend(deList(i))
            else:
                objects.append(i)
        return objects
                
    objects = deList(args)    
    cmd = ""
    for i in objects:
        if type(i) == str:
            cmd += i
            cmd += " "
        elif type(i) == FILE or type(i) == DIR:
            cmd += i.path
            cmd += " "
    return cmd

def checkDir(Dir):
    if not os.path.exists(Dir.path):
        try:
            os.makedirs(Dir.path, exist_ok=True)
            return True
        except:
            return False
    else:
        if os.path.isdir(Dir.path):
            return True
        else:
            return False

def run(command):
    print(command)
    print("")
    if os.system(command):
        sys.exit(1)

def compileCFile(CFile, cc, flags, outputDir):
    if not checkDir(outputDir):
        return
    RelFile = FILE(os.path.join(outputDir.path, CFile.filename.replace(".c", ".rel")))
    command = makeCmd(cc, flags, "-c", CFile, "-o", RelFile)
    run(command)
    return RelFile

def assembleRelFiles(RelFileslist, outputDir, target):
    command = makeCmd(CC, FLAGS, RelFileslist, "-o", target.path)
    run(command)

def upload(targetFile, comPort):
    command = makeCmd("stcgal", "-p", comPort, targetFile)
    run(command)

def clearDir(dir, clearihx = False):
    dir.refresh()
    ableToRemove = [".asm", ".rel", ".lst", ".map", ".mem", ".sym", ".lk", ".rst", ".cdb"]
    if clearihx: ableToRemove.append(".ihx")
    
    print("Removing:", ableToRemove)
    
    for i in dir.file:
        if i.path.endswith(tuple(ableToRemove)):
            os.remove(i.path)
    

makefileRAWPath = os.path.abspath(__file__)
makefileRAWDir = os.path.dirname(makefileRAWPath)

makefileDir = DIR(makefileRAWDir)
makefileName = os.path.basename(makefileRAWPath)

######################Default parameters

COM = "COM16"
TARGET = "Result.ihx"

OUTPUT_DIR = DIR(os.path.join(makefileDir.path, "output"))
TARGET_FILE = FILE(os.path.join(OUTPUT_DIR.path, TARGET))

CC = "sdcc"
FLAGS = "-mmcs51 --model-small --stack-auto --code-size 8192"
SRCS = getAllCfiles(makefileDir)

######################Cmd parameter processing

try:
    cmd = sys.argv[1]
except:
    cmd = "all"

if cmd == "all":
    RelFileslist = []
    for i in SRCS:
        RelFileslist.append(compileCFile(i, CC, FLAGS, OUTPUT_DIR))
    assembleRelFiles(RelFileslist, OUTPUT_DIR, TARGET_FILE)
    clearDir(OUTPUT_DIR)
    upload(TARGET_FILE, COM)
if cmd == "clearall":
    clearDir(OUTPUT_DIR, clearihx = True)