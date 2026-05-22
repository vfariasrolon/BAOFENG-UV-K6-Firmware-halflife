Import("env")
import shutil
import os
import glob
import datetime

def postBuild(source, target, env):
    print("Running postBuild script:")
    binFile = target[0].get_abspath()
    print("Bin file Path:", binFile)
    projectDir = os.getcwd()
    outDir = os.path.join(projectDir, "out")
    print("Project directory: " + projectDir)
    print("Out directory: " + outDir)
    
    env_name = env.get("PIOENV", "default")
    
    # create out directory if it doesn't exist
    if not os.path.exists(outDir):
        os.makedirs(outDir)

    outFile = os.path.join(outDir, f"bfk6-{env_name}.bin")

    # Remove all old files with the same env name
    files = glob.glob(outDir + f"/bfk6-{env_name}*.bin")
    for f in files:
        os.remove(f)

    # Copy bin file to out directory
    shutil.copy(binFile, outFile) 


env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", postBuild)