
#
# Usage: python install_console.py
#

# Maybe?
import os
from shutil import copytree, rmtree, copyfile
import subprocess


if not os.getenv("CASUAL_BUILD_HOME") or not os.getenv("CASUAL_HOME"):
	raise SystemError, "CASUAL-environment need to be set"



console_home =  os.getenv("CASUAL_HOME", "/tmp/casual/build") + "/console"
console_build_home = os.getenv("CASUAL_BUILD_HOME", '/tmp/casual') + "/console"

server_src =  console_build_home + "/server"
server_dest = console_home + "/server"




client_src =  console_build_home + "/client/dist"
client_dest = console_home + "/client"

if os.path.exists(client_dest):
  rmtree(client_dest)

print("Copying console frontend client")
copytree(client_src, client_dest)

nginx_src =  console_build_home + "/nginx"
nginx_dest = os.getenv("CASUAL_HOME", "/tmp/casual/build") + "/nginx/conf"


print("Copying nginx-config")
copyfile(nginx_src + "/web.console.conf", nginx_dest + "/web.console.conf")


if os.path.exists(server_dest):
  rmtree(server_dest)
  
copytree(server_src, server_dest)



os.chdir(server_dest)
print("Installing python dependencies for admin console")
print("Running pip install")
print(subprocess.check_output([
  'pip', 'install', '--target=lib', '-r', 'requirements.txt'
]))
print("Installed")

