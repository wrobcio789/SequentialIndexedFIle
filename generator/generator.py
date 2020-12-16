import random

f = open("input3.txt", "w")

INSERTS_COUNT = 10*1000
READ_COUNT = 10*1000
DELETE_COUNT = 10*1000

MAX_KEY = 50 * 1000 * 1000

allValues = list(range(1, MAX_KEY))
random.shuffle(allValues)
allUsedValues = allValues[:INSERTS_COUNT]


uninsertedValues = allUsedValues
random.shuffle(uninsertedValues)
for value in uninsertedValues:
    f.write(f"insert {value} {value} {value} {value} {value} {value}\n")
f.write("statistics\n")
f.write("reset_statistics\n")


unreadValues = allUsedValues
random.shuffle(unreadValues)
for value in unreadValues:
    f.write(f"find {value}\n")
f.write("statistics\n")
f.write("reset_statistics\n")


undeletedValues = allUsedValues
random.shuffle(undeletedValues)
for value in undeletedValues:
    f.write(f"delete {value}\n")
f.write("statistics\n")
f.write("reset_statistics\n")


f.write("quit\n")
f.close()
