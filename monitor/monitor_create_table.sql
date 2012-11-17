CREATE TABLE calls
(
	service			TEXT PRIMARY KEY,
	parentservice	TEXT,
	callchainid		BLOB,
	transactionid	BLOB,
	starttime		TEXT,
	endtime			TEXT
);