{
  'conditions': [
    ['OS=="win"', {
      'variables': {
        'MAGICK_ROOT%': '<!(python get_regvalue.py)',
        # download the dll binary and check off for libraries and includes
        'OSX_VER%': "0",
      }
    }],
	],
  "targets": [  
    {
      "target_name": "<(module_name)",
      "sources": [
	  "CImageReader.cpp",
	  "CImageReader.h",
	  "qtAddons.cc"
	  ],
	  "defines": [
		"QT_DEPRECATED_WARNINGS",
		"QT_NO_DEBUG",
		"QT_GUI_LIB",
		"QT_CORE_LIB"
	  ],
	  "include_dirs": [
		 "../quazip",
		 "../libopc/include",
		 "<(MAGICK_ROOT)/include",
		 "$(QTDIR)/include",
		 "$(QTDIR)/include/QtGui",
		 "$(QTDIR)/include/QtANGLE",
		 "$(QTDIR)/include/QtCore",
		 "$(QTDIR)/mkspecs/win32-msvc"
		 ],
	  "link_settings": {
		 "libraries": [
		 "../quazip/release/quazip.lib",
		 "../libopc/lib/mce.lib",
		 "../libopc/lib/opc.lib",
		 "../libopc/lib/plib.lib",
		 "../libopc/lib/xml.lib",
		 "../libopc/lib/zlib.lib",
		 "../magick/lib/CORE_RL_bzlib_.lib",
		 "../magick/lib/CORE_RL_cairo_.lib",
		 "../magick/lib/CORE_RL_coders_.lib",
		 "../magick/lib/CORE_RL_croco_.lib",
		 "../magick/lib/CORE_RL_exr_.lib",
		 "../magick/lib/CORE_RL_ffi_.lib",
		 "../magick/lib/CORE_RL_flif_.lib",
		 "../magick/lib/CORE_RL_glib_.lib",
		 "../magick/lib/CORE_RL_jp2_.lib",
		 "../magick/lib/CORE_RL_jpeg_.lib",
		 "../magick/lib/CORE_RL_lcms_.lib",
		 "../magick/lib/CORE_RL_libde265_.lib",
		 "../magick/lib/CORE_RL_libheif_.lib",
		 "../magick/lib/CORE_RL_liblzma_.lib",
		 "../magick/lib/CORE_RL_libraw_.lib",
		 "../magick/lib/CORE_RL_librsvg_.lib",
		 "../magick/lib/CORE_RL_libxml_.lib",
		 "../magick/lib/CORE_RL_lqr_.lib",
		 "../magick/lib/CORE_RL_MagickCore_.lib",
		 "../magick/lib/CORE_RL_openjpeg_.lib",
		 "../magick/lib/CORE_RL_pango_.lib",
		 "../magick/lib/CORE_RL_pixman_.lib",
		 "../magick/lib/CORE_RL_png_.lib",
		 "../magick/lib/CORE_RL_tiff_.lib",
		 "../magick/lib/CORE_RL_ttf_.lib",
		 "../magick/lib/CORE_RL_webp_.lib",
		 "../magick/lib/CORE_RL_zlib_.lib",
		 "../magick/lib/CORE_RL_jbig_.lib",
		 "../magick/lib/CORE_RL_flif_.lib",
		 "$(QTDIR)/lib/Qt5Gui.lib",		 
		 "$(QTDIR)/lib/Qt5Core.lib"
		 ],
		}
    },
	{
      "target_name": "action_after_build",
      "type": "none",
      "dependencies": [ "<(module_name)" ],
      "copies": [
        {
          "files": [ "<(PRODUCT_DIR)/DllOffice.node" ],
          "destination": "<(module_path)"
        }
      ],
      "copies": [
        {
          "files": [ "<(PRODUCT_DIR)/<(module_name).node" ],
          "destination": "<(module_path)"
        }
      ]
    }
	]
}
