# NAME: Clayton Green, Anthony Humay
# EMAIL: clayton.green26@gmail.com, ahumay@ucla.edu
# ID: 404546151, 304731856

#!/usr/bin/python
import csv
import sys
import os

#
# SCAN CSV
#

def scanValid(curBlock, levelString, blockString, inodeString, offsetString, nodeNum, curOffset, lvl):
	newData = [nodeNum, curOffset, lvl]
	if not 0 < curBlock < totalBlcks:
		print("INVALID " + levelString + blockString + inodeString + offsetString)
		EXITSTATUS = 2
	elif reservedList.count(curBlock) != 0:
		print("RESERVED " + levelString + blockString + inodeString + offsetString)
		EXITSTATUS = 2
	elif dictionary.get(curBlock) == None:
		newEntry = [newData]
		dictionary[curBlock] = newEntry
	else:
		dictionary[curBlock].append(newData)

def DIRENT(curRow, totalNodes): 
	prnt = int(curRow[1])
	logOffset = int(curRow[2])
	nodeNum = int(curRow[3])
	entryLen = int(curRow[4])
	dirNameLen = int(curRow[5])
	dirName = curRow[6]
	curDirIndex = 3
	formerDirIndex = 4

	nodesReferenced[nodeNum] = dirName

	dirString = "DIRECTORY INODE " + str(prnt)
	nameString = " NAME "
	invalidnodeString = " INVALID INODE " + str(nodeNum)
	if nodeNum >= 1:
		pass
	if nodeNum <= totalNodes: 
		pass
	else:
		nameString = nameString + dirName[0 : dirNameLen]
		print(dirString + nameString + invalidnodeString)
		EXITSTATUS = 2
		return

	linkNodeString = " LINK TO INODE "
	parentNodeString = " SHOULD BE "
	if nodeRefLinkCount.get(nodeNum) != None:
		nodeRefLinkCount[nodeNum] = nodeRefLinkCount[nodeNum] + 1
	else:
		nodeRefLinkCount[nodeNum] = 1

	if openList.count(nodeNum) != 0:
		validLink[nodeNum] = 1
	else:
		validLink[nodeNum] = -1

	if dirName[0 : curDirIndex] == "'.'":
		return
	elif dirName[0 : formerDirIndex] == "'..'":
		nodeParentNumber[prnt] = nodeNum
	elif dirName[0 : curDirIndex] == "'.'":
		if prnt != nodeNum:
			nameString = nameString + '.'
			linkNodeString = linkNodeString + str(nodeNum)
			parentNodeString = parentNodeString + str(prnt) 
			print(dirString + nameString + linkNodeString + parentNodeString)
			EXITSTATUS = 2
	else:
		nodeParent[nodeNum] = prnt

	return

def INDIRECT(curRow, totalBlcks):
	nodeNum = int(curRow[1])
	lvl = int(curRow[2])
	blockOffset = int(curRow[3])
	blockNumScan = int(curRow[4])
	curBlock = int(curRow[5])

	curOffset = FIRSTLEVELOFFSET
	if lvl == 1:
		levelString = "INDIRECT "
	elif lvl == 2:
		levelString = "DOUBLE INDIRECT "
		curOffset = SECONDLEVELOFFSET
	elif lvl == 3:
		levelString = "TRIPLE INDIRECT "
		curOffset = THIRDLEVELOFFSET

	blockString = "BLOCK " + str(curBlock)
	inodeString = " IN INODE " + str(nodeNum)
	offsetString = " AT OFFSET " + str(curOffset)
	scanValid(curBlock, levelString, blockString, inodeString, offsetString, nodeNum, curOffset, lvl)

	return

def analyze_csv(openFD):
	analyzedFlag = True
	data = openFD.readlines()
	global totalBlcks
	global totalNodes
	for row in data:
		curRow = row.split(",")
		version = curRow[0]

		if version == "SUPERBLOCK":
			totalBlcks = int(curRow[1])
			totalNodes = int(curRow[2])
		elif version == "BFREE":
			freeBlcks.add(int(curRow[1])) 
		elif version == "IFREE":
			freeNodes.add(int(curRow[1])) 
		elif version == "DIRENT":
			DIRENT(curRow, totalNodes)
		elif version == "INDIRECT":
			INDIRECT(curRow, totalBlcks)
		elif version == "INODE":
			nodeNum = int(curRow[1])
			ownerNum = int(curRow[4])
			groupNum = int(curRow[5])
			nodeLinkCount[nodeNum] = int(curRow[6])
			row = 12
			while row < 24:
				curBlock = int(curRow[row])
				if curBlock != 0:
					curOffset = 0
					lvl = 0
					levelString = ""

					blockString = "BLOCK " + str(curBlock)
					inodeString = " IN INODE " + str(nodeNum)
					offsetString = " AT OFFSET " + str(curOffset)
					scanValid(curBlock, levelString, blockString, inodeString, offsetString, nodeNum, curOffset, lvl)
				row += 1

			row = 24
			while row < 27:
				curBlock = int(curRow[row])
				if curBlock != 0:
					curOffset = 0
					if row == 24:
						lvl = 1
						levelString = "INDIRECT "
						curOffset = FIRSTLEVELOFFSET
					elif row == 25:
						lvl = 2
						levelString = "DOUBLE INDIRECT "
						curOffset = SECONDLEVELOFFSET
					elif row == 26:
						lvl = 3
						levelString = "TRIPLE INDIRECT "
						curOffset = THIRDLEVELOFFSET

					blockString = "BLOCK " + str(curBlock)
					inodeString = " IN INODE " + str(nodeNum)
					offsetString = " AT OFFSET " + str(curOffset)
					scanValid(curBlock, levelString, blockString, inodeString, offsetString, nodeNum, curOffset, lvl)
				row += 1
	return

#
# CHECK CSV
#

def check_directs():
	for dir_par in nodeParentNumber:
		if 2 == dir_par == nodeParentNumber[dir_par]:
			continue
		else:
			if dir_par == 2:
				EXITSTATUS = 2
				first = third = str(2)
				second = str(nodeParentNumber[dir_par])
				print("DIRECTORY INODE " + first + " NAME '..' LINK TO INODE " + second + " SHOULD BE " + third)
			elif dir_par not in nodeParent:
				EXITSTATUS = 2
				first = str(nodeParentNumber[dir_par])
				second = str(dir_par)
				third = str(nodeParentNumber[dir_par])
				print("DIRECTORY INODE " + first + " NAME '..' LINK TO INODE " + second + " SHOULD BE " + third)
			elif nodeParentNumber[dir_par] != nodeParent[dir_par]: 
				EXITSTATUS = 2
				first = second = str(dir_par)
				third = str(nodeParentNumber[dir_par])
				print("DIRECTORY INODE " + first + " NAME '..' LINK TO INODE " + second + " SHOULD BE " + third)				

def check_inodes():
	valids = (1, 3, 4, 5, 6, 7, 8, 9, 10)
	spread = range(1, totalNodes + 1)
	for num in spread:
		num_string = str(num)
		if num in nodeLinkCount:
			if num in freeNodes:
				EXITSTATUS = 2
				print("ALLOCATED INODE " + num_string + " ON FREELIST")
		elif not (num in valids or num in nodeLinkCount or num in freeNodes or num in nodeParent):
			EXITSTATUS = 2
			print("UNALLOCATED INODE " + num_string + " NOT ON FREELIST")

	for i_no in nodesReferenced:
		if i_no in freeNodes:
			if i_no in nodeParent:
				EXITSTATUS = 2
				i_no_str = str(i_no)
				i_no_par = str(nodeParent[i_no])
				i_no_dir = nodesReferenced[i_no]
				end_index = (len(i_no_dir) - 2)
				i_no_dir_str = i_no_dir[0:end_index]
				print("DIRECTORY INODE " + i_no_par + " NAME " + i_no_dir_str + "' UNALLOCATED INODE " + i_no_str)

def check_linkcounts():
	for key in nodeLinkCount:
		act_lc = 0
		if key in nodeRefLinkCount:
			act_lc = nodeRefLinkCount[key]

		key_str = str(key)
		act_lc_str = str(act_lc)
		if act_lc != nodeLinkCount[key]:
			EXITSTATUS = 2
			lc_str = str(nodeLinkCount[key])
			print("INODE " + key_str + " HAS " + act_lc_str + " LINKS BUT LINKCOUNT IS " + lc_str)

def check_blocks_allocated_unreferenced():
	spread = range(1, totalBlcks + 1, 1)
	for num in spread:
		if num in dictionary:
			if num in freeBlcks:
				EXITSTATUS = 2
				curBlock = str(num)
				print("ALLOCATED BLOCK " + curBlock + " ON FREELIST")
		elif not (reservedList.count(num) != 0 or num in dictionary or num in freeBlcks):
			EXITSTATUS = 2
			curBlock = str(num)
			print("UNREFERENCED BLOCK " + curBlock)

def check_blocks_duplicates():
	for single in dictionary:
		length = len(dictionary[single])
		if length > 1:
			EXITSTATUS = 2
			for data in dictionary[single]:
				curBlock = str(single)
				nodeNum = str(data[0])
				curOffset = str(data[1])
				lvl = int(data[2])
				if 3 == lvl:
					print("DUPLICATE TRIPLE INDIRECT BLOCK " + curBlock + " IN INODE " + nodeNum + " AT OFFSET " + curOffset)
				elif 2 == lvl:
					print("DUPLICATE DOUBLE INDIRECT BLOCK " + curBlock + " IN INODE " + nodeNum + " AT OFFSET " + curOffset)
				elif 1 == lvl:
					print("DUPLICATE INDIRECT BLOCK " + curBlock + " IN INODE " + nodeNum + " AT OFFSET " + curOffset)
				elif 0 == lvl:
					print("DUPLICATE BLOCK " + curBlock + " IN INODE " + nodeNum + " AT OFFSET " + curOffset)


#
# MAIN
#

dictionary = {} 
nodesReferenced = {}
nodeRefLinkCount = {}
validLink = {}
nodeLinkCount = {} 
reservedList = [0, 1, 2, 3, 4, 5, 6, 7, 64, 128, 256]
openList = [4, 12, 24, 36, 48, 60]
freeNodes = set()
EXITSTATUS = 0
analyzedFlag = False
FIRSTLEVELOFFSET = 12
SECONDLEVELOFFSET = 268
THIRDLEVELOFFSET = 65804
nodeParent = {}
nodeParentNumber = {}
totalBlcks = 0
totalNodes = 0
freeBlcks = set()

def main():
	global dictionary
	global nodeRefLinkCount
	global nodeLinkCount
	global validLink
	global nodesReferenced
	global analyzedFlag
	global openList
	global nodeParent
	global nodeParentNumber
	numArgs = len(sys.argv)
	if numArgs != 2:
		sys.stderr.write("\nERROR: Improper number of arguments (must be 1).\n")
		exit(1)

	fileName = sys.argv[1]
	openFD = open(fileName, 'r')
	if openFD == 0:
		print_error("\nERROR: File could not be opened.\n")

	analyze_csv(openFD)
	check_blocks_allocated_unreferenced()
	check_blocks_duplicates()
	check_inodes()
	check_directs()
	check_linkcounts()

	openFD.close()

	exit(EXITSTATUS)

if __name__ == '__main__':
	main()