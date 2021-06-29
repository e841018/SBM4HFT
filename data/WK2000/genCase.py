# Generate complete graph cases with WK2000
numVars = int(input("Input desired variable number: "))
numEdges = numVars * (numVars-1) // 2
f = open("WK2000_1.rud", "r")
newfName = f"WK{numVars}_1.rud"
newf = open(newfName, "w")
newf.write(str(numVars) + " " + str(numEdges) + "\n")
firstline = f.readline().split()

lineCount = 0
while lineCount < numEdges:
    newLine = f.readline()
    line = newLine.split()
    if int(line[1]) <= numVars:
        newf.write(newLine)
        lineCount += 1
