mkdir files
mkdir output

pip install matplotlib
sudo apt-get update
sudo apt-get install postgresql postgresql-contrib
sudo apt-get install libpq-dev
sudo apt-get install uuid-dev

sudo -u postgres psql -c "CREATE DATABASE autograder;"
sudo -u postgres psql -c "CREATE USER grader_user WITH ENCRYPTED PASSWORD 'password';"
sudo -u postgres psql -c "ALTER ROLE grader_user SET client_encoding TO 'utf8';"
sudo -u postgres psql -c "ALTER ROLE grader_user SET default_transaction_isolation TO 'read committed';"
sudo -u postgres psql -c "ALTER ROLE grader_user SET timezone TO 'UTC';"
sudo -u postgres psql -c "GRANT ALL PRIVILEGES ON DATABASE autograder TO grader_user;"
sudo -u postgres psql -d autograder -c  "CREATE TABLE IF NOT EXISTS grading_requests (id uuid primary key, status int, error text null default null);"
sudo -u postgres psql -d autograder -c "GRANT ALL PRIVILEGES ON TABLE grading_requests TO grader_user;"

sudo sed -i "s/ssl = on/ssl = off/" /etc/postgresql/16/main/postgresql.conf

sudo service postgresql restart

make