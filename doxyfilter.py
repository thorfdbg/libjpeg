#!/usr/bin/python

import fileinput
import string
import re

commentblock = 0
emptycomment = re.compile("^//$")
fullcomment  = re.compile("^//.")
foldmarker   = re.compile("^///")
hppfile      = re.compile("^.*\.hpp$")
for line in fileinput.input():
	if hppfile.match(fileinput.filename()):
		line  = string.expandtabs(line)
		token = string.strip(line)
		indent = len(line) - len(string.lstrip(line))
		if (indent > 0):
			blanks = string.ljust("",indent-1)
		else:
			blanks = ""
		if emptycomment.match(token):
			continue
		if fullcomment.match(token) and not foldmarker.match(token):
			if commentblock:
				print re.sub("//","  ",line),
			else:
				print
				print blanks,
				print "/*!"
				print re.sub("//","  ",line),
				commentblock = 1
		else:
			if commentblock:
				print blanks,
				print " */"
				print line,
				commentblock = 0
			else:
				print line,
	else:
		print line,


