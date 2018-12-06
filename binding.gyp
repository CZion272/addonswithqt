{
  "targets": [
    {
      "target_name": "<(module_name)",
      "sources": [
	  "CImageReader.cpp",
	  "CImageReader.h",
	  "qtAddons.cc",
	  ],
	  "defines": [
		"QT_DEPRECATED_WARNINGS",
		"QT_NO_DEBUG",
		"QT_GUI_LIB",
		"QT_CORE_LIB",
	  ],
	  "include_dirs": [
		 "./quazip",
		 "$(QTDIR)/include",
		 "$(QTDIR)/include/QtGui",
		 "$(QTDIR)/include/QtANGLE",
		 "$(QTDIR)/include/QtCore",
		 "$(QTDIR)/mkspecs/win32-msvc"
		 ],
	  "link_settings": {
		 "libraries": [
		 "quazip.lib",
		 "$(QTDIR)/lib/Qt5Guid.lib",
		 "$(QTDIR)/lib/Qt5Cored.lib"
		 ],
		}
    },
	{
      "target_name": "action_after_build",
      "type": "none",
      "dependencies": [ "<(module_name)" ],
      "copies": [
        {
          "files": [ "<(PRODUCT_DIR)/<(module_name).node" ],
          "destination": "<(module_path)"
        }
      ]
    }
	]
}
