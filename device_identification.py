# python script to handle device ID

import numpy as np
import mltools as ml
import sys
from pymongo import MongoClient

MONGO_URI = 'mongodb://sparkystrip:calplug123@ds011429.mlab.com:11429/sparkystrip'
MONGO_DATABASE = 'sparkystrip'


mongo_connect = MongoClient(MONGO_URI)
mongo_db = mongo_connect[MONGO_DATABASE]
mongo_collection = None

np.random.seed(0)

created_files = []

training_names = []
if len(sys.argv) >= 2:
    for i in range(1, len(sys.argv)):
        training_names.append(sys.argv[i].strip())

try:
    with open('training_X', 'w') as X, open('training_Y', 'w') as Y: 
        for training_name in training_names:
            mongo_collection = mongo_db[training_name]
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
                    Y.write(training_name + '\n')
except:
    print("Failed operation.")

X_train = np.genfromtext("training_X", delimiter = ',')
Y_train = np.genfromtext("training_Y", delimiter = ',')

X_predict = np.genfromtext("predict_X", delimiter = ',')

lr = ml.linear.linearRegress(X_train, Y_train)
Y_predict = lr.predict(X_predict)

with open('predict_Y', 'w') as pre_Y:
    for Y_i in Y_predict:
        pre_Y.write(str(Y_i[0] + '\n'))
