import os
import sys
import subprocess
import shutil
import platform
import glob
import re
import tarfile
GREEN = "\033[0;32m"
YELLOW = "\033[0;33m"
RED = "\033[0;31m"
NC = "\033[0m"
LIGHTBLUE = "\033[1;34m"

workSpaceDir = os.path.dirname(os.path.abspath(__file__))
nameOfScript = os.path.basename(__file__) + " (python version)"
scriptAuthor = "(c) Tomáš Mark 2004"
scriptVersion = "0.0.1"

taskName = sys.argv[1] if len(sys.argv) > 1 else None
buildArch = sys.argv[2] if len(sys.argv) > 2 else None
buildType = sys.argv[3] if len(sys.argv) > 3 else "Not Defined"
isCrossCompilation = False

buildFolderName = "Build"
installOutputDir = os.path.join(workSpaceDir, buildFolderName, "Install")
artefactsOutputDir = os.path.join(workSpaceDir, buildFolderName, "Artefacts")
valid_archs = ["default", "x86_64-unknown-linux-gnu", "x86_64-w64-mingw32", "aarch64-linux-gnu", "independent_parameter"]
valid_build_types = ["Debug", "Release", "RelWithDebInfo", "MinSizeRel"]

def exit_ok(msg):
    print(f"{GREEN}{msg}{NC}")
    sys.exit(0)

def exit_with_error(msg):
    print(f"{RED}{msg}{NC}")
    sys.exit(1)

if not taskName:
    exit_with_error("Task name is missing. Exiting.")

# Print out the welcom and configuration
print(f"{YELLOW}{nameOfScript} {scriptAuthor} v {scriptVersion} {NC}")
print(f"{LIGHTBLUE}taskName\t: {taskName}{NC}")
print(f"{GREEN}Build Arch\t: {buildArch}")
print(f"Build Type\t: {buildType}")
print(f"Work Space\t: {workSpaceDir}{NC}")
print(f"Install to\t: {installOutputDir}{NC}")
print(f"Artefacts to\t: {artefactsOutputDir}{NC}")

isCrossCompilation = False
if buildArch in valid_archs:
    isCrossCompilation = (buildArch != "default")
else:
    if "darwin" in platform.system().lower():
        isCrossCompilation = False
    else:
        exit_with_error("Undefined build architecture. Exiting.")

print(f"{YELLOW}Cross\t\t: {isCrossCompilation}{NC}")

### Log to file, revision 1
def log2file(message):
    with open(os.path.join(workSpaceDir, "SolutionController.log"), "a") as f:
        f.write(message + "\n")

### Execute command, revision 1
def execute_command(cmd):
    print(f"{LIGHTBLUE}> Executed: {cmd}{NC}")
    log2file(cmd)
    if platform.system().lower() == "windows":
        result = subprocess.run(cmd, shell=True)
    else:
        result = subprocess.run(cmd, shell=True, executable="/bin/bash")
    if result.returncode != 0:
        exit_with_error(f"Command failed: {cmd}")

### Execute subprocess with shell, revision 1
def execute_subprocess(cmd, executable):
    print(f"{LIGHTBLUE}> Executed: {cmd}{NC}")
    log2file(cmd)
    result = subprocess.run(cmd, shell=True, executable=executable)
    if result.returncode != 0:
        exit_with_error(f"Command failed: {cmd}")

def get_build_dir(kind):
    return os.path.join(buildFolderName, kind, buildArch, buildType)

### Conan install, revision 2
def conan_install(bdir):
    with open("CMakeLists.txt") as f:
        cmake_content = f.read()
    shared_flag = "-o *:shared=True" if 'option(BUILD_SHARED_LIBS "Build using shared libraries" ON)' in cmake_content else "-o *:shared=False"
    profile = "default" if not isCrossCompilation else buildArch
    exeCmd = f'conan install "{workSpaceDir}" --output-folder="{os.path.join(workSpaceDir, bdir)}" --build=missing --profile {profile} --settings build_type={buildType} {shared_flag}'
    execute_command(exeCmd)

### CMake configuration, revision 2
def cmake_configure(src, bdir):
    
    conan_toolchain_file_path = os.path.join(workSpaceDir, bdir, "conan_toolchain.cmake")
    
    # Conan
    # ---------------------------------------------------------------------------------
    if os.path.isfile(conan_toolchain_file_path):
        print(f"{LIGHTBLUE} using file: conan_toolchain.cmake {NC}")
        print(f"{LIGHTBLUE} using file:", conan_toolchain_file_path, NC)
        DCMAKE_TOOLCHAIN_FILE_CMD = f'-DCMAKE_TOOLCHAIN_FILE="{conan_toolchain_file_path}"'

        if platform.system().lower() in ["linux", "darwin"]: 
            # CMake configuration for Linux and MacOS with Conan toolchain
            conan_build_sh_file = os.path.join(workSpaceDir, bdir, "conanbuild.sh")
            bashCmd = f'source "{conan_build_sh_file}" && cmake -S "{src}" -B "{os.path.join(workSpaceDir, bdir)}" {DCMAKE_TOOLCHAIN_FILE_CMD} -DCMAKE_BUILD_TYPE={buildType} -DCMAKE_INSTALL_PREFIX="{os.path.join(installOutputDir, buildArch, buildType)}"'
            execute_subprocess(bashCmd, "/bin/bash")
        
        if platform.system().lower() == "windows":

            # CMake configuration for Windows x64 with Conan toolchain    
            conan_build_bat_file = os.path.join(workSpaceDir, bdir, "conanbuild.bat")
            winCmd = f'call "{conan_build_bat_file}" && cmake -S "{src}" -B "{os.path.join(workSpaceDir, bdir)}" {DCMAKE_TOOLCHAIN_FILE_CMD} -DCMAKE_BUILD_TYPE={buildType} -DCMAKE_INSTALL_PREFIX="{os.path.join(installOutputDir, buildArch, buildType)}"'
            execute_subprocess(winCmd, "cmd.exe")

    # CMake solo
    # This command condition will miss find_package(Conan's packages) in CMakeLists.txt
    # But it is useful for CMake projects without Conan
    # ---------------------------------------------------------------------------------
    if not os.path.isfile(conan_toolchain_file_path):
        if buildArch in valid_archs and buildArch != "default":
            # CMake configuration for cross-compilation with solo toolchain
            cmake_toolchain_file = os.path.join(workSpaceDir, "Utilities", "CMakeToolChains", buildArch + ".cmake")
            DCMAKE_TOOLCHAIN_FILE_CMD = f"-DCMAKE_TOOLCHAIN_FILE=" + cmake_toolchain_file
            print(f"{LIGHTBLUE} using file:", cmake_toolchain_file, NC)
        else:
            # CMake native
            DCMAKE_TOOLCHAIN_FILE_CMD = ""   
        # CMake solo command
        cmd = f'cmake -S "{src}" -B "{os.path.join(workSpaceDir, bdir)}" {DCMAKE_TOOLCHAIN_FILE_CMD} -DCMAKE_BUILD_TYPE={buildType} -DCMAKE_INSTALL_PREFIX="{os.path.join(installOutputDir,buildArch,buildType)}"'
        execute_command(cmd)



### CMake build, revision 3
def cmake_build(bdir, target=None):
    if target is None:
        target = ""
    else:
        target = f"--target {target}"    
        
    conan_build_sh_file = os.path.join(workSpaceDir, bdir, 'conanbuild.sh')
    if os.path.exists(conan_build_sh_file):
        bashCmd = f'source "{conan_build_sh_file}" && cmake --build "{os.path.abspath(bdir)}" {target} -j {os.cpu_count()}'
        #bashCmd = f'cmake --build "{os.path.abspath(bdir)}" {target} -j {os.cpu_count()}'
    else:
        bashCmd = f'cmake --build "{os.path.abspath(bdir)}" {target} -j {os.cpu_count()}'
    execute_subprocess(bashCmd, "/bin/bash")



### Clean build folder, revision 1   
def clean_build_folder(bdir):
    print(f"{LIGHTBLUE}> Removing build directory: {bdir}{NC}")
    log2file(f"Remove: {bdir}")
    shutil.rmtree(bdir, ignore_errors=True)


def build_spltr(lib, st):
    if lib:
        cmake_build(get_build_dir("Library"))
    if st:
        cmake_build(get_build_dir("Standalone"))

def license_spltr(lib, st):
    if lib:
        cmake_build(get_build_dir("Library"), "write-licenses")
    if st:
        cmake_build(get_build_dir("Standalone"), "write-licenses")

def configure_spltr(lib, st):
    if lib:
        cmake_configure(".", get_build_dir("Library"))
    if st:
        cmake_configure("./Standalone", get_build_dir("Standalone"))
        
def cmake_install(bdir):
    cmake_build(bdir, target="install")        

def conan_spltr(lib, st):
    if lib:
        conan_install(get_build_dir("Library"))
    if st:
        conan_install(get_build_dir("Standalone"))

def clean_spltr(lib, st):
    if lib:
        clean_build_folder(get_build_dir("Library"))
    if st:
        clean_build_folder(get_build_dir("Standalone"))

def install_spltr(lib, st):
    if lib:
        cmake_install(get_build_dir("Library"))
    if st:
        cmake_install(get_build_dir("Standalone"))

def get_version_and_names():
    with open('CMakeLists.txt', 'r') as file:
        cmake_content = file.read()
    with open('Standalone/CMakeLists.txt', 'r') as file:
        standalone_content = file.read()

    lib_ver = re.search(r'VERSION\s+(\d+\.\d+\.\d+)', cmake_content).group(1)
    lib_name = re.search(r'set\(LIBRARY_NAME\s+(\w+)', cmake_content).group(1)
    st_name = re.search(r'set\(STANDALONE_NAME\s+(\w+)', standalone_content).group(1)

    return lib_ver, lib_name, st_name

def create_archive(source_dir, out_path):
    with tarfile.open(out_path, "w:gz") as tar:
        tar.add(source_dir, arcname=os.path.basename(source_dir))
    print(f"Created archive: {out_path}")

def artefacts_spltr(lib, st):
    os.makedirs(artefactsOutputDir, exist_ok=True)
    lib_ver, lib_name, st_name = get_version_and_names()
    
    print(f"buildArch: {buildArch}")
    print(f"buildType: {buildType}")
    print(f"artefactsOutputDir: {artefactsOutputDir}")
    print(f"valid_archs: {valid_archs}")
    
    if buildArch in valid_archs:
        if lib:
            archive_name = f"{lib_name}-{lib_ver}-{buildArch}-{buildType}.tar.gz"
            source_dir = os.path.join(installOutputDir, buildArch, buildType)
            
            if os.listdir(source_dir):
                print(f"Creating archive for library from: {source_dir}")
                out_path = os.path.join(artefactsOutputDir, archive_name)
                create_archive(source_dir, out_path)
            else:
                print(f"No content found in {source_dir} for library.")
            
        if st:
            st_archive_name = f"{st_name}-{lib_ver}-{buildArch}-{buildType}.tar.gz"
            source_dir = os.path.join(installOutputDir, buildArch, buildType)
            
            if os.listdir(source_dir):
                print(f"Creating archive for standalone from: {source_dir}")
                out_path = os.path.join(artefactsOutputDir, st_archive_name)
                create_archive(source_dir, out_path)
            else:
                print(f"No content found in {source_dir} for standalone.")
                
def lint_c():
    # build dirs for json compilation database is required
    bdir_lib = get_build_dir("Library")
    bdir_st = get_build_dir("Standalone")

    def run_clang_tidy(bdir):
        for root, _, files in os.walk(workSpaceDir):
            if "Build" in root:
                continue
            for file in files:
                if file.endswith((".c", ".cpp", ".h", ".hpp")):
                    full_path = os.path.join(root, file)
                    cmd = f'clang-tidy -p "{bdir}" "{full_path}"'
                    print(f"Linting: {full_path}")
                    execute_command(cmd)
                    print(f"Done: {full_path}")

    run_clang_tidy(bdir_lib)
    # run_clang_tidy(bdir_st)
    
def format_clang():
    for root, _, files in os.walk(workSpaceDir):
        if "Build" in root:
            continue
        for file in files:
            if file.endswith((".c", ".cpp", ".h", ".hpp")):
                full_path = os.path.join(root, file)
                cmd = f'clang-format -i "{full_path}"'
                print(f"Processing: {full_path}")
                execute_command(cmd)
                print(f"Done: {full_path}")

def format_cmake():
    for root, _, files in os.walk(workSpaceDir):
        if "Build" in root or "cmake" in root or "Utilities" in root:
            continue
        for file in files:
            if file == "CMakeLists.txt" or file.endswith(".cmake"):
                full_path = os.path.join(root, file)
                cmd = f'cmake-format --enable-markup -i "{full_path}"'
                print(f"Processing: {full_path}")
                execute_command(cmd)
                print(f"Done: {full_path}")

def permutate_all_tasks():
    shutil.rmtree("Build", ignore_errors=True)
    for arch in valid_archs:
        if arch == "independent_parameter":
            continue
        for t in valid_build_types:
            global buildArch, buildType
            buildArch = arch
            buildType = t
            clean_spltr(True, True)
            conan_spltr(True, True)
            configure_spltr(True, True)
            build_spltr(True, True)
            license_spltr(True, True)
            install_spltr(True, True)
            artefacts_spltr(True, True)

def conan_graph():
    cmd = f'conan graph info "{workSpaceDir}" --format=html > graph.html'
    execute_command(cmd)

task_map = {
    "Zero to Hero 🦸": lambda: (clean_spltr(True, True), conan_spltr(True, True), configure_spltr(True, True), build_spltr(True, True), exit_ok("")),
    "📚 Zero to Hero 🦸": lambda: (clean_spltr(True, False), conan_spltr(True, False), configure_spltr(True, False), build_spltr(True, False), exit_ok("")),
    "🎯 Zero to Hero 🦸": lambda: (clean_spltr(False, True), conan_spltr(False, True), configure_spltr(False, True), build_spltr(False, True), exit_ok("")),
    "Clean 🧹": lambda: (clean_spltr(True, True), exit_ok("")),
    "📚 Clean 🧹": lambda: (clean_spltr(True, False), exit_ok("")),
    "🎯 Clean 🧹": lambda: (clean_spltr(False, True), exit_ok("")),
    "Conan 🗡️": lambda: (conan_spltr(True, True), exit_ok("")),
    "📚 Conan 🗡️": lambda: (conan_spltr(True, False), exit_ok("")),
    "🎯 Conan 🗡️": lambda: (conan_spltr(False, True), exit_ok("")),
    "Configure 🔧": lambda: (configure_spltr(True, True), exit_ok("")),
    "📚 Configure 🔧": lambda: (configure_spltr(True, False), exit_ok("")),
    "🎯 Configure 🔧": lambda: (configure_spltr(False, True), exit_ok("")),
    "Build 🔨": lambda: (build_spltr(True, True), exit_ok("")),
    "📚 Build 🔨": lambda: (build_spltr(True, False), exit_ok("")),
    "🎯 Build 🔨": lambda: (build_spltr(False, True), exit_ok("")),
    "Collect Licenses 📜": lambda: (license_spltr(True, True), exit_ok("")),
    "📚 Collect Licenses 📜": lambda: (license_spltr(True, False), exit_ok("")),
    "🎯 Collect Licenses 📜": lambda: (license_spltr(False, True), exit_ok("")),
    "Install Artefacts 📌": lambda: (install_spltr(True, True), exit_ok("")),
    "📚 Install Artefacts 📌": lambda: (install_spltr(True, False), exit_ok("")),
    "🎯 Install Artefacts 📌": lambda: (install_spltr(False, True), exit_ok("")),
    "Release Artefacts 📦": lambda: (artefacts_spltr(True, True), exit_ok("")),
    "📚 Release Artefacts 📦": lambda: (artefacts_spltr(True, False), exit_ok("")),
    "🎯 Release Artefacts 📦": lambda: (artefacts_spltr(False, True), exit_ok("")),
    "Permutate All Tasks 🕧": lambda: (permutate_all_tasks(), exit_ok("")),
    "⚔️ Conan graph.html": lambda: (conan_graph(), exit_ok("")),
    "🔍 Lint C/C++ files": lambda: (lint_c(), exit_ok("")),
    "📐 Format C/C++ files (Clang)": lambda: (format_clang(), exit_ok("")),
    "📏 Format CMake files": lambda: (format_cmake(), exit_ok("")),
    "": lambda: exit_ok("")
}

if taskName in task_map:
    task_map[taskName]()
else:
    print(f"Received unknown task: {taskName}")
    exit_with_error("Task name is missing. Exiting.")
    
# Copyright (c) 2024 Tomáš Mark