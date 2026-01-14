# Sensor Lab Project

## Project Structure
```bash
project/
├── server/                 # Flask backend source code
├── firmware/               # Microcontroller firmware
├── hardware/               # Circuit PCB and schematic 
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
1. Clone the project
Clone the repository and navigate into the directory:
```bash
git clone https://github.com/hphuc15/LightSense_IoT.git
cd .\LightSense_IoT
```
2. Configure Environment Variables
Create the .env file inside the server/ directory and fill it with your database connection details.

On Windows:
```bash
notepad .\server\.env # You can edit by any others text editor
```
On Linux, MacOS:
```bash
nano .\server\.env
```

Fill in your DB information:
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