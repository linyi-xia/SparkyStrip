import sys
from pymongo import MongoClient

MONGO_URI = 'mongodb://sparkystrip:calplug123@ds011429.mlab.com:11429/sparkystrip'
MONGO_DATABASE = 'sparkystrip'


mongo_connect = MongoClient(MONGO_URI)
mongo_db = mongo_connect[MONGO_DATABASE]
database_collection = None

created_files = []

if len(sys.argv) == 2:
    database_collection = sys.argv[1].strip()
else:
    print("Invalid amount of arguments.")
    sys.exit(1)

mongo_collection = mongo_db[database_collection]

try:
    with open('predict_X', 'w') as X: 
        for post in mongo_collection.find():
            d = dict(post)
            if d['Type'] == 'Current':
                line = ''
                for i in range(len(d['Real'])):
                    line += str(d['Real'][i]) + ', '
                for i in range(len(d['Imaginary'])):
                    line += str(d['Imaginary'][i]) + ', '
                line += str(d['Power']) + ', '
                line += str(d['Power Factor'])
                X.write(line + '\n')
except:
    print("Failed operation.")

mongo_collection.drop()
