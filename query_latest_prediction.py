from pymongo import MongoClient, DESCENDING

MONGO_URI = 'mongodb://sparkystrip:calplug123@ds011429.mlab.com:11429/sparkystrip'
MONGO_DATABASE = 'sparkystrip'

mongo_connect = MongoClient(MONGO_URI)
mongo_db = mongo_connect[MONGO_DATABASE]
mongo_collection = mongo_db['PREDICTIONS']

try:
    for post in mongo_collection.find().sort([("Time", DESCENDING)]).limit(1):
        d = dict(post)
        print(d['Device Name'])
except Exception as e:
    print("Failed operation:",str(e))

