# note that each device will have it’s own login and will only be granted permission to call pushData()
# the device’s username will be u1234@x where 1234 is derived from the mac address


DROP DATABASE IF EXISTS SparkyStrip;

CREATE DATABASE SparkyStrip;

USE SparkyStrip;

CREATE TABLE Appliances(
	appName VARCHAR(80),
	d1 FLOAT DEFAULT 0,
	d2 FLOAT DEFAULT 0,
	d3 FLOAT DEFAULT 0,
	d4 FLOAT DEFAULT 0,
	d5 FLOAT DEFAULT 0,
	d6 FLOAT DEFAULT 0,
	d7 FLOAT DEFAULT 0,
	d8 FLOAT DEFAULT 0,
	d9 FLOAT DEFAULT 0,
	d10 FLOAT DEFAULT 0,
	trainCount INT DEFAULT 0,
	PRIMARY KEY (appName)
);

CREATE TABLE AppUpdates(
	updateNum INT
);
INSERT INTO AppUpdates(updateNum) 
	VALUES(0);

CREATE TABLE Devices(
	devID INT, 
	lastStateChange TIME NOT NULL, 
	appName VARCHAR(80),
	lastPush TIME,
	PRIMARY KEY (devID),
	FOREIGN KEY(appName) REFERENCES Appliances(appName) ON DELETE SET NULL
); 

CREATE TABLE RawData(
	dataID INT AUTO_INCREMENT,
	devID INT NOT NULL, 
	dateTime TIMESTAMP NOT NULL,
	d1 FLOAT NOT NULL,
	d2 FLOAT NOT NULL,
	d3 FLOAT NOT NULL,
	d4 FLOAT NOT NULL,
	d5 FLOAT NOT NULL,
	d6 FLOAT NOT NULL,
	d7 FLOAT NOT NULL,
	d8 FLOAT NOT NULL,
	d9 FLOAT NOT NULL,
	d10 FLOAT NOT NULL,
	PRIMARY KEY (dataID),
	FOREIGN KEY(devID) REFERENCES Devices(devID) ON DELETE CASCADE
);

CREATE TABLE DeviceHistory(
	dataID INT, 
	appName VARCHAR(80),
	PRIMARY KEY (dataID, appName),
	FOREIGN KEY(dataID) REFERENCES RawData(dataID) ON DELETE CASCADE,
	FOREIGN KEY(appName) REFERENCES Appliances(appName) ON DELETE CASCADE
);

CREATE TABLE Unprocessed(
	dataID INT,
	PRIMARY KEY (dataID),
	FOREIGN KEY(dataID) REFERENCES RawData(dataID) ON DELETE CASCADE
);

CREATE TABLE UserDevices(
	userID VARCHAR(80),
	devID INT,
	PRIMARY KEY (userID, devID),
	FOREIGN KEY(devID) REFERENCES Devices(devID) ON DELETE CASCADE
);

##### Stored Procedures ######


### helpers 

# set the username into @ username
# trims off the @xxx part from user()
DELIMITER $$
CREATE PROCEDURE setUsername()
BEGIN
	DECLARE at_loc int;
	SELECT LOCATE('@', user() )
		INTO at_loc;
	SELECT left(user(), at_loc-1 ) 
		INTO @Username;
END $$
DELIMITER ;


# set the devID into @devID
# this one is called by pushData 
DELIMITER $$
CREATE PROCEDURE setDevID()
BEGIN
	DECLARE devID_string VARCHAR(80);
	CALL setUsername();
	SELECT SUBSTR(@Username, 2) 
		INTO devID_string;
	SELECT CAST(devID_string AS UNSIGNED) 
		INTO @DevID;
END $$
DELIMITER ;

# returns null if device not associate, sent value if so
DELIMITER $$
CREATE FUNCTION checkDeviceID(deviceID INT) RETURNS INT 
	DETERMINISTIC
BEGIN
	DECLARE ans INT;
	CALL setUsername();
	SELECT devID 
		FROM UserDevices 
		WHERE userID = @Username AND devID = deviceID 
		INTO ans;
	RETURN ans;
END $$
DELIMITER ;


### Main Procedures

# called by the device, inserts the data in RawData and puts the dataID in the Unprocessed table
DELIMITER $$
CREATE PROCEDURE pushData(
	IN d_1 FLOAT,
	IN d_2 FLOAT,
	IN d_3 FLOAT,
	IN d_4 FLOAT,
	IN d_5 FLOAT,
	IN d_6 FLOAT,
	IN d_7 FLOAT,
	IN d_8 FLOAT,
	IN d_9 FLOAT,
	IN d_10 FLOAT)
BEGIN
	DECLARE _time TIME;
	DECLARE data_ID, train_count, next_count, i INT;
	DECLARE app_name, field VARCHAR(80);
	
	set _time = NOW();
	
	# ensure the variable is set
	IF @DevID IS NULL THEN
		CALL setDevID();
	END IF;
	
	# Insert the data
	INSERT INTO RawData(devID, dateTime, d1, d2, d3, d4, d5, d6, d7, d8, d9, d10)
		VALUES(@DevID, _time, d_1, d_2, d_3, d_4, d_5, d_6, d_7, d_8, d_9, d_10);
	
	# get the dataID from the insertion
	SELECT LAST_INSERT_ID() 
		INTO data_ID;
		
	# update our activity tracking
	UPDATE Devices 
		SET lastPush = _time
		WHERE devID = @DevID;
		
	# Find if we are training, and what the appliance name is
	SELECT appName 
		FROM Devices 
		WHERE devID = @DevID
		INTO app_name;
		
	IF app_name IS NULL THEN
		# we are not training so this data will need to be identified
		# put it in the unprocessed table
		INSERT 
			INTO Unprocessed(dataID) 
			VALUES (data_ID);
	ELSE
		# training, so lets handle it here
		INSERT INTO DeviceHistory(dataID,appName) VALUES(data_ID,app_name);
		
		# get the count so far
		SELECT trainCount 
			FROM Appliances 
			WHERE appName = app_name 
			INTO train_count;
		set next_count = train_count + 1;
		
		# update mean values
		UPDATE Appliances
			SET d1 = (d1 * train_count + d_1) /next_count,
				d2 = (d2 * train_count + d_2) /next_count,
				d3 = (d3 * train_count + d_3) /next_count,
				d4 = (d4 * train_count + d_4) /next_count,
				d5 = (d5 * train_count + d_5) /next_count,
				d6 = (d6 * train_count + d_6) /next_count,
				d7 = (d7 * train_count + d_7) /next_count,
				d8 = (d8 * train_count + d_8) /next_count,
				d9 = (d9 * train_count + d_9) /next_count,
				d10 = (d10 * train_count + d_10) /next_count,
				trainCount = next_count
			WHERE appName = app_name;
			
		IF next_count = 15 THEN
			# we train with 15 samples so stop the training here
			UPDATE Devices 
				SET appName = NULL
				WHERE devID = @DevID;
			# up our update count
			UPDATE AppUpdates
				SET updateNum = updateNum + 1;
		END IF;			
		
	END IF;
END$$
DELIMITER ;


# this one is used by the server to get data that needs processing, deals with the Unprocessed table so the server doesn’t need to.
DELIMITER $$
CREATE PROCEDURE getUnprocessed()
BEGIN
	DECLARE data_ID, dev_ID, train_count, update_num INT;
	# get 1 dataID (oldest by the way it’s structured)
	SELECT dataID
		FROM Unprocessed 
		LIMIT 1 
		INTO data_ID;

	# remove it from the unprocessed table
	DELETE 
		FROM Unprocessed 
		WHERE dataID = data_ID;
	
	# return the relevant info
	SELECT updateNum, dataID, d1, d2, d3, d4, d5, d6, d7, d8, d9, d10 
		FROM RawData, AppUpdates
		WHERE dataID = data_ID;
END$$
DELIMITER ;

	
# used by the app to get the history of a device - with a limit (1 returns just most recent)
DELIMITER $$
CREATE PROCEDURE getHistory(
	IN deviceID INT,
	IN histLimit INT
)
BEGIN
	IF checkDeviceID(deviceID) IS NULL THEN
		SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'ERROR: Device not associated with user!';
	ELSE
		SELECT dateTime, appName, d1 as power 
			FROM (SELECT * FROM RawData WHERE devID = deviceID) as temp1
				NATURAL JOIN DeviceHistory
			ORDER BY dateTime DESC
			LIMIT histLimit;
	END IF;
END$$
DELIMITER ;

# setup training mode (which happens within mysql)
DELIMITER $$
CREATE PROCEDURE setTraining(
	IN deviceID INT,
	IN applianceName VARCHAR(80)
)
BEGIN
	IF checkDeviceID(deviceID) IS NULL THEN
		SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = 'ERROR: Device not associated with user!';
	ELSE
		IF applianceName IS NOT NULL THEN
			DELETE 
				FROM Appliances 
				WHERE appName = applianceName;
			IF ROW_COUNT() != 0 THEN
				# we deleted an appliance so up our update count
				UPDATE AppUpdates
					SET updateNum = updateNum + 1;
			END IF;
			INSERT 
				INTO Appliances(appName) 
					VALUES(applianceName);
		END IF;
		UPDATE Devices 
			SET lastStateChange = NOW(),
				appName = applianceName
			WHERE devID = deviceID;
	END IF;
END$$
DELIMITER ;

# returns the status of all devices associate with the user
DELIMITER $$
CREATE PROCEDURE viewDevices()
BEGIN
	CALL setUsername();
	SELECT devID, lastStateChange, appName as trainingAppliance, lastPush 
		FROM Devices
		NATURAL JOIN (SELECT * 
			FROM UserDevices
			WHERE userID = @Username) as temp1;
END$$
DELIMITER ;



### Create initial users

DROP USER IF EXISTS 'u123'@'%';
CREATE USER 'u123'@'%' 
	IDENTIFIED BY 'sparkystrip_device';
GRANT 
	EXECUTE ON PROCEDURE SparkyStrip.pushData 
	TO 'u123'@'%';

DROP USER IF EXISTS 'Dpynes'@'%';	
CREATE USER 'Dpynes'@'%' 
	IDENTIFIED BY 'Pizzahead9';
GRANT 
	EXECUTE ON PROCEDURE SparkyStrip.setTraining
	TO 'Dpynes'@'%';
GRANT 
	EXECUTE ON PROCEDURE SparkyStrip.viewDevices 
	TO 'Dpynes'@'%';
GRANT 
	EXECUTE ON PROCEDURE SparkyStrip.getHistory
	TO 'Dpynes'@'%';

INSERT INTO Devices(devID, lastStateChange) 
	VALUES(123, NOW());
INSERT INTO UserDevices(userID, devID) 
	VALUES('Dpynes', 123);
	
DROP USER IF EXISTS 'sparkyID'@'%' ;
CREATE USER 'sparkyID'@'%' 
	IDENTIFIED BY 'squidsofpink';
GRANT 
	EXECUTE ON PROCEDURE SparkyStrip.getUnprocessed
	TO 'sparkyID'@'%';
GRANT 
	INSERT ON SparkyStrip.DeviceHistory 
	TO 'sparkyID'@'%';
GRANT 
	SELECT ON SparkyStrip.Appliances 
	TO 'sparkyID'@'%';



