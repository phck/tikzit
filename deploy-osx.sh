# deploy the Mac app bundle. Note the bin/ directory
# of Qt should be in your PATH

# copy in libraries and set (most) library paths
macdeployqt tikzit.app

# create DMG
hdiutil create -volname TikZiT -srcfolder tikzit.app -ov -format UDZO tikzit.dmg


