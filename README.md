# Sensor Lab Project

## Project Structure
```bash
project/
├── server/                 # Flask backend source code
├── firmware/               # Microcontroller firmware
├── database/               # Database
├── docs/                   # Project documentation
├── test/                   
├── requirements.txt        # Python dependencies list
├── .env                    # Environment variables
└── .gitignore              # Git ignore rules
```

### server/

### firmware/

### database/

### docs

### test

### requirements.txt

### .env
```.env
DB_USER = <database_user>
DB_PASSWORD = <database_password>
DB_NAME = <database_name>
```

### .gitignore
```.gitignore
venv/
.env
```


## How to use?
### Clone the project to your local
If you use Windows:
```bash
git clone https://github.com/hphuc15/LightSense_IoT.git
cd .\LightSense_IoT
notepad .\server\.env
```
Fill your DB information:
```.env
DB_USER = <database_user>
DB_PASSWORD = <database_password>
DB_NAME = <database_name>
```

Set up venv:
```bash
python -m venv venv
venv\Scripts\activate
pip install -r requirements.txt
```