pip install matplotlib
sudo apt install -y postgresql

sudo -u postgres psql -c "CREATE DATABASE autograder;"
sudo -u postgres psql -c "CREATE USER autograder WITH ENCRYPTED PASSWORD 'password';"
sudo -u postgres psql -c "ALTER ROLE autograder SET client_encoding TO 'utf8';"
sudo -u postgres psql -c "ALTER ROLE autograder SET default_transaction_isolation TO 'read committed';"
sudo -u postgres psql -c "ALTER ROLE autograder SET timezone TO 'UTC';"
sudo -u postgres psql -c "GRANT ALL PRIVILEGES ON DATABASE autograder TO autograder;"

# make