-- premake5.lua
workspace "ThunderBall"
   configurations { "Debug", "Release" }
   startproject "ThunderBall"

includedirs { "." }

configuration "windows"
   includedirs { "$(VK_SDK_PATH)/Include", "external/glfw-3.2.1/include", "external/glm-0.9.9-a2" }
   libdirs { "$(VK_SDK_PATH)/Lib" }
   if _ACTION == "vs2012" then
     libdirs { "external/glfw-3.2.1/lib-vc2012" }
   elseif _ACTION == "vs2013" then
     libdirs { "external/glfw-3.2.1/lib-vc2013" }
   else
     libdirs { "external/glfw-3.2.1/lib-vc2015" }
   end
   links { "vulkan-1.lib", "glfw3" }
     defines { "WIN32" }
   architecture "x86_64"  


configuration "vs*"
--defines { "_CRT_SECURE_NO_WARNINGS" }
 
project "ThunderBall"
   location "source"
   kind "ConsoleApp"
   language "C++"
   targetdir "bin/%{cfg.buildcfg}"
   debugdir "."

   pchheader "stdafx.h"
   pchsource "source/stdafx.cpp"

   files 
   { 
      "source/**.h", 
      "source/**.cpp",
      "shaders/**.vert",   
      "shaders/**.frag",     
   }

   filter "configurations:Debug"
      defines { "DEBUG" }
      symbols  "On"
      libdirs { "lib/Debug" }

   filter "configurations:Release"
      defines { "NDEBUG" }
      optimize "On"
      libdirs { "lib/Release" }

filter 'files:**.vert or files:**.frag'
   -- A message to display while this build step is running (optional)
   buildmessage 'Compiling %{file.relpath}'

   -- One or more commands to run (required)
   buildcommands {
   --   'glslangValidator -V -o "%{file.basename}%{file.extension}.spv" "%{file.relpath}"'
      'glslangValidator -V -o "%{file.relpath}.spv" "%{file.relpath}"'
   }

   -- One or more outputs resulting from the build (required)
   --buildoutputs { '%{file.basename}%{file.extension}.spv' }
   buildoutputs { '%{file.relpath}.spv' }
