## 一、MySQL相关配置
### 1. 安装 MySQL：

#### Ubuntu/Debian 系统：

```bash
sudo apt-get update
sudo apt-get install mysql-server
```

#### CentOS/RHEL 系统：

```bash
sudo yum install mysql-server
```

### 2. 启动 MySQL 服务：

#### Ubuntu/Debian 系统：

```bash
sudo systemctl start mysql
```

#### CentOS/RHEL 系统：

```bash
sudo systemctl start mysqld
```

### 3. 运行 MySQL 安全性脚本（可选但推荐）：

```bash
sudo mysql_secure_installation
```

该脚本会提示你设置MySQL的root密码、删除匿名用户、禁止root远程登录等。按照提示进行操作。

### 4. 登录到 MySQL 控制台：

```bash
mysql -u root -p
```

### 5. 创建用户和数据库：

```sql
CREATE DATABASE your_database_name;
CREATE USER 'your_user'@'localhost' IDENTIFIED BY 'your_password';
GRANT ALL PRIVILEGES ON your_database_name.* TO 'your_user'@'localhost';
FLUSH PRIVILEGES;
```

确保替换 `your_database_name`、`your_user` 和 `your_password` 为实际的数据库名、用户名和密码。

### 6. 配置 MySQL 服务器：

MySQL的配置文件通常是 `/etc/mysql/my.cnf` 或 `/etc/my.cnf`，你可以根据你的系统查找该文件。

### 7. 重启 MySQL 服务以使更改生效：

#### Ubuntu/Debian 系统：

```bash
sudo systemctl restart mysql
```

#### CentOS/RHEL 系统：

```bash
sudo systemctl restart mysqld
```

这些步骤提供了一个基本的 MySQL 配置。你还可以根据需求进一步调整配置，例如调整缓冲池、日志等参数。

请注意，确保在配置 MySQL 时考虑安全性，并在生产环境中采取适当的安全措施，例如限制远程访问、使用强密码等。

## 运行

### 命令行参数
```bash
mysql_config --libs
```

```bash
gcc crud.c -o crud -L/usr/lib64/mysql -lmysqlclient
```