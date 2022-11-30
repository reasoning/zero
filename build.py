import re
import sys
import os


if __name__ == "__main__":

	print("Zero Build Script")

	def execute(*args):
		if len(args) == 1 and type(args[0]) is not list:
			args = [args[0]]
		for arg in args:
			#os.system("%s > make.log 2>&1" % arg)
			os.system("%s" % arg)

	def expand(*args):
		if len(args) == 1 and type(args[0]) is not list:
			args = [args[0]]
			os.system("tar zxvf %s > /dev/null" % args)
		else:
			os.system("tar zxvf %s -C %s > /dev/null" % (args[0], args[1]))
		#os.chdir(tar[:-len(".tar.gz")])

	def buildZero():

		# We only have one dependency for now, but as this project grows we expand the build
		# requirements.
		def buildLibUV():
			print("\t\tBuilding LibUV")

			if not os.path.exists("./libuv"):
				
				def clone():
					execute("git clone https://github.com/libuv/libuv.git")

				def release():
					version = "1.44.2"
					if not os.path.exists("./libuv/libuv.tar.gz"):
						# Wget the tagged 1.44.2 release and output to libuv.tar.gz
						execute("wget https://github.com/libuv/libuv/archive/refs/tags/v%s.tar.gz -O libuv.tar.gz" % version)

					# Internally this will extract to a subfolder called libuv-1.44.2 
					# which we can then rename to our desired library name
					expand("libuv.tar.gz","./")			
					execute("mv libuv-%s libuv" % version)
			
			# We can clone the git repo or extract a tagged release
			clone()
			#release()

			os.chdir("libuv")			
			execute("cmake .")
			execute("make")
			execute("ln -s libuv_a.a libuv.a")
			os.chdir("../")

		
		

		# Change to the lib directory so we can build all the dependencies, then drop
		# back into the current directory to run the makefile for zero
		if not os.path.exists("lib"):
			os.mkdir("lib")

		os.chdir("lib")
		
		buildLibUV()

		os.chdir("../")		
		
		print("\tBuilding Zero")

		execute("make clean","make depend","make")


	# Any build process can be broken into simple step by step instructions
	# usually involving extracting archives, executing commmands and changing
	# directories, manipulating files, and adding symlinks.  
	buildZero()





		
