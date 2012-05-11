#!/usr/bin/python

import sys
from xml.dom.minidom import parse

if len(sys.argv) != 3:
	print "Please pass the name of an svg and the name of a group to not hide."

dom = parse(sys.argv[1])

found = False

def supress(elt):
	global found
	for child in elt.childNodes:
		if elt.localName == 'g' and child.localName == 'title':
			name = child.firstChild.data
			if name != sys.argv[2]:
				elt.setAttribute('style', 'visibility:hidden;display:none')
				sys.stderr.write("Hiding %s.\n" % name)
			else:
				sys.stderr.write("Leaving %s as is.\n" % name)
				found = True
		else:
			supress(child)

supress(dom)

if not found:
	sys.stderr.write("Title %s not found.\n" % sys.argv[2])
	exit(-1)

sys.stdout.write(dom.toxml(encoding="utf-8"));
