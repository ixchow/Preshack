#!/usr/bin/perl -w
#
# Build a .app hierarchy.
#

if (@ARGV != 2) {
	print "Please pass application name, 4cc.\n";
	return;
}

my $name = $ARGV[0];
my $short = $ARGV[1];
length($short) == 4 || die "$short not four characters";

my $id = $name;
$id =~ s/\s/_/g;

print <<END;
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple Computer//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
	<key>CFBundleDevelopmentRegion</key>
	<string>English</string>
	<key>CFBundleExecutable</key>
	<string>$name</string>
	<key>CFBundleIconFile</key>
	<string>$name.icns</string>
	<key>CFBundleIdentifier</key>
	<string>com.tchow.$id</string>
	<key>CFBundleInfoDictionaryVersion</key>
	<string>6.0</string>
	<key>CFBundleName</key>
	<string>$name</string>
	<key>CFBundlePackageType</key>
	<string>APPL</string>
	<key>CFBundleSignature</key>
	<string>$short</string>
	<key>CFBundleVersion</key>
	<string>1.0</string>
</dict>
</plist>
END
