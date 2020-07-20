import logging
import os
import re
import winreg
import zipfile


def find_steam():
    REG_64_PATH = r"SOFTWARE\Wow6432Node\Valve\Steam"
    REG_32_PATH = r"SOFTWARE\Valve\Steam"
    try:
        key = winreg.OpenKey(winreg.HKEY_LOCAL_MACHINE, REG_32_PATH)
    except FileNotFoundError:
        key = winreg.OpenKey(winreg.HKEY_LOCAL_MACHINE, REG_64_PATH)
    return winreg.QueryValueEx(key, "InstallPath")[0]


def get_steam_libraries(steam_path):
    libfolders = os.path.join(steam_path, r"SteamApps\libraryfolders.vdf")
    with open(libfolders) as f:
        folders = [steam_path]
        lf = f.read()
        folders.extend([fn.replace("\\\\", "\\") for fn in
                        re.findall('^\s*"\d*"\s*"([^"]*)"', lf, re.MULTILINE)])
    return folders


def get_f4_doc_dir():
    if (USERPROFILE := os.path.expanduser("~")) == "~":
        logging.error("Could not fetch USERPROFILE")
    elif not os.path.exists(F4_DOC_DIR := os.path.join(USERPROFILE, r"Documents\My Games\Fallout4")):
        logging.error(f"{F4_DOC_DIR} does not exist, please try starting up your game at least once.")
    else:
        return F4_DOC_DIR
    raise Exception


def get_f4_dir(libs):
    for lib in steam_libs:
        loc = os.path.join(lib, r"steamapps\common\Fallout 4")
        if os.path.exists(loc):
            break
    else:
        logging.error("Could not find Fallout 4 Install location")
        raise Exception
    return loc


if __name__ == '__main__':
    logging.basicConfig(filename="latest.log", filemode="w", format="[%(levelname)s] %(message)s", level=logging.DEBUG)
    doc_dir = get_f4_doc_dir()
    logging.info(f"Fallout 4 Config dir: {doc_dir}")
    steam_libs = get_steam_libraries(find_steam())
    logging.info(f"Steam Libs dir: {steam_libs}", )
    fallout4_dir = get_f4_dir(steam_libs)
    logging.info(f"Fallout 4 dir: {fallout4_dir}")
    f4se_dir = os.path.join(fallout4_dir, "f4se_loader.exe")
    if not os.path.exists(f4se_dir):
        logging.error("F4SE is not installed.")
    else:
        logging.info(f"F4SE Installed at: {f4se_dir}")

    zip_file = zipfile.ZipFile("logs.zip", "w", zipfile.ZIP_DEFLATED)
    config = os.path.join(doc_dir, r"F4MP\config.txt")
    f4se_log = os.path.join(doc_dir, r"F4SE\f4se.log")
    papyrus_log = os.path.join(doc_dir, r"Logs\Script\Papyrus.0.log")
    logging.info(f"Writing {config}")
    try:
        zip_file.write(config, os.path.basename(config))
    except FileNotFoundError:
        logging.error("Missing config")
    logging.info(f"Writing {f4se_log}")
    try:
        zip_file.write(f4se_log, os.path.basename(f4se_log))
    except FileNotFoundError:
        logging.error("Missing f4se_log")
    logging.info(f"Writing {papyrus_log}")
    try:
        zip_file.write(papyrus_log, os.path.basename(papyrus_log))
    except FileNotFoundError:
        logging.error("Missing papyrus_log")
    logging.info("Finished getting logs")
    del logging
    zip_file.write("latest.log", os.path.basename("latest.log"))

