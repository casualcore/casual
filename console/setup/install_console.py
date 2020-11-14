
#
# Usage: python install_console.py
#

# Maybe?

print("Installing casual admin console backend")

import os
from shutil import copytree, rmtree, copyfile
import subprocess


if not os.getenv("CASUAL_BUILD_HOME") or not os.getenv("CASUAL_HOME"):
	raise SystemError, "CASUAL-environment need to be set"



console_home =  os.getenv("CASUAL_HOME", "/tmp/casual/build") + "/console"
console_build_home = os.getenv("CASUAL_BUILD_HOME", '/tmp/casual') + "/console"

server_src =  console_build_home + "/server"
server_dest = console_home + "/server"



nginx_src =  console_build_home + "/nginx"
nginx_dest = os.getenv("CASUAL_HOME", "/tmp/casual/build") + "/nginx/conf"


print("Copying nginx-config")
copyfile(nginx_src + "/web.console.conf", nginx_dest + "/web.console.conf")


if os.path.exists(server_dest):
  rmtree(server_dest)
  
copytree(server_src, server_dest)



os.chdir(server_dest)
os.mkdir("lib")
print("Installing python dependencies for admin console")
print("Running pip install")
print(subprocess.check_output([
  'pip', 'install', '--target=lib', '-r', 'requirements.txt'
]))
print("Installed")

