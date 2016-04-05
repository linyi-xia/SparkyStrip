DROP DATABASE SparkyStrip;

CREATE DATABASE SparkyStrip;

USE SparkyStrip;

CREATE TABLE Appliances(
name VARCHAR(80),
active FLOAT,
reactive FLOAT,
real60 FLOAT,
real180 FLOAT,
real360 FLOAT,
real420 FLOAT,
imm60 FLOAT,
imm180 FLOAT,
imm360 FLOAT,
imm420 FLOAT,
PRIMARY KEY (name)
);

CREATE TABLE Devices(
devID INT, 
lastChange TIMESTAMP NOT NULL, 
appID INT,
PRIMARY KEY (devID),
FOREIGN KEY(appID) REFERENCES Appliances(appID) ON DELETE SET NULL
); 

CREATE TABLE RawData(
dataID INT AUTO_INCREMENT,
devID INT, 
dateTime TIMESTAMP,
active FLOAT NOT NULL,
reactive FLOAT NOT NULL,
real60 FLOAT NOT NULL,
real180 FLOAT NOT NULL,
real360 FLOAT NOT NULL,
real420 FLOAT NOT NULL,
imm60 FLOAT NOT NULL,
imm180 FLOAT NOT NULL,
imm360 FLOAT NOT NULL,
imm420 FLOAT NOT NULL,
PRIMARY KEY (dataID),
FOREIGN KEY(devID) REFERENCES Devices(devID) ON DELETE CASCADE
);

CREATE TABLE DeviceHistory(
devID INT, 
name VARCHAR(80),
PRIMARY KEY (dataID, name),
FOREIGN KEY(dataID) REFERENCES RawData(dataID) ON DELETE CASCADE,
FOREIGN KEY(name) REFERENCES Appliances(name) ON DELETE CASCADE
);

CREATE TABLE UnprocessedData(
dataID INT,
PRIMARY KEY (dataID),
FOREIGN KEY(dataID) REFERENCES RawData(dataID) ON DELETE CASCADE
);


DELIMITER $$
CREATE PROCEDURE insertData(
IN Active FLOAT,
IN Reactive FLOAT,
IN Real60 FLOAT,
IN Real180 FLOAT,
IN Real360 FLOAT,
IN Real420 FLOAT,
IN Imm60 FLOAT,
IN Imm180 FLOAT,
IN Imm360 FLOAT,
IN Imm420 FLOAT)

BEGIN
# get the time
SELECT CURRENT_TIMESTAMP() INTO @current_time;

# Get the devID (username is u1234@x where 1234 is the devID)
SELECT USER() INTO @user;
SELECT LOCATE('@', @user ) INTO @at_loc;
SELECT MID(@user, 2, @at_loc-2 ) INTO @devID;

# Insert the data
INSERT INTO RawData(devID, dateTime, active, reactive, real60, real180, real360, real420, imm60, imm180, imm360, imm420)
VALUES(@devID, @current_time, Active, Reactive, Real60, Real180, Real360, Real420, Imm60, Imm180, Imm360, Imm420);

# get the dataID and add it to UnprocessedData
SELECT LAST_INSERT_ID() INTO @dataID;
INSERT INTO UnprocessedData(dataID) VALUES (@dataID);

END$$
DELIMITER ;







