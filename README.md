# ODBC
利用odbc连接达梦数据库。
# kylinv10-server sp3 环境搭建及编译
## 一、数据库安装
数据库安装可以选择达梦数据库、MySQL数据库。
### 1.1 达梦数据库安装

1.1.1 创建用户及权限修改
```shell
groupadd dinstall
useradd -m -g dinstall -s /bin/bash dmdba
passwd dmdba
mkdir -p /dm8
chown dmdba:dinstall /dm8
systemctl stop firewalld
systemctl disable firewalld
setenforce 0
```
1.1.2 修改：/etc/security/limits.conf，加入下面内容
```text
dmdba soft nofile 65536
dmdba hard nofile 65536
dmdba soft nproc 65536
dmdba hard nproc 65536
```
1.1.3 解压安装达梦
```shell
unzip dm8_20260710_x86_kylin10_sp3_64.zip
mkdir -p /mnt/dm
mount -o loop ./dm8_20260710_x86_kylin10_sp3_64.iso /mnt/dm
su - dmdba
cd /mnt/dm
chmod +x ./DMInstall.bin
./DMInstall.bin -i
请选择安装目录 [/home/dmdba/dmdbms]:/dm8
```
1.1.4 创建数据库实例
```shell
su - dmdba
cd /dm8/bin
./dminit path=/dm8/data db_name=DAMENG instance_name=DMSERVER SYSDBA_PWD=YourStrongPwd123 SYSAUDITOR_PWD=YourAuditorPwd123
su - root
cd /dm8/script/root/
./dm_service_installer.sh -t dmserver -dm_ini /dm8/data/DAMENG/dm.ini -p DMSERVER
systemctl start DmServiceDMSERVER.service
systemctl enable DmServiceDMSERVER.service
```
1.1.5 修改dmdba环境变量/home/dmdba/.bashrc
```text
export DM_HOME=/dm8
export PATH=$DM_HOME/bin:$PATH
```
### 1.2 安装MySQL
1.2.1 清理冲突文件
```shell
yum remove mariadb-server mariadb-client
yum module disable mysql -y
```
1.2.2 安装mysql
```shell
#wget https://dev.mysql.com/get/mysql80-community-release-el8-8.noarch.rpm
yum localinstall mysql80-community-release-el8-8.noarch.rpm
yum localinstall *.rpm
```
1.2.3 启动mysql并设置开机自启
```shell
systemctl start mysqld
systemctl enable mysqld
```
1.2.4 设置密码
```shell
#获取临时密码
grep 'temporary password' /var/log/mysqld.log
mysql -uroot -p
ALTER USER 'root'@'localhost' IDENTIFIED BY '你的新密码';
```
1.2.5 安装 mysql-connector-odbc
```shell
yum localinstall mysql-connector-odbc-26.10.0-1.el8.x86_64.rpm
```
## 二、驱动安装
2.1 查看当前odbc驱动
```shell
[root@localhost root]# rpm -qa | grep unixODBC
unixODBC-2.3.7-3.h1.ky10.x86_64
```
2.2 安装驱动
```shell
yum install unixODBC-devel
```
## 三、配置软件
查询odbc配置文件位置。
```shell
odbcinst -j
```
### 3.1 达梦数据库配置
3.1.1 在 odbcinst.ini 加入以下配置：
```text
[DM8 ODBC DRIVER]
Description = ODBC Driver for DM8
Driver = /dm8/bin/libdodbc.so
```
3.1.2 编辑/root/.odbc.ini
```text
[DM8_TEST]
Description = DM ODBC DSN
Driver = DM8 ODBC DRIVER
SERVER = localhost
UID = SYSDBA
PWD = 你的SYSDBA密码
TCP_PORT = 5236
```
3.1.3 测试连接
```shell
isql -v DM8_TEST
```
### 3.2.MySQL配置
3.2.1 在 odbcinst.ini 加入以下配置（安装mysql-connector后默认就有MySQL的配置）：
```shell
[MySQL ODBC 26.10 Unicode Driver]
DRIVER=/usr/lib64/libmyodbc26w.so
UsageCount=1

[MySQL ODBC 26.10 ANSI Driver]
DRIVER=/usr/lib64/libmyodbc26a.so
UsageCount=1

```
3.2.2 编辑/root/.odbc.ini
```shell
[MySQL_DSN]
Description = MySQL Database
Driver      = MySQL ODBC 26.10 Unicode Driver
Server      = 127.0.0.1
Port        = 3306
User        = root
Password    = Ckjs220807*
Database    = 你的数据库名
CharSet     = utf8mb4
```
3.2.3 测试连接
```shell
isql -v MySQL_DSN
```

## 四、编译器安装
4.1 g++安装
```shell
yum install gcc-c++
```
4.2 安装编译工具cmake

略...

## 五、编译
```shell
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```
## 六、运行
```shell
./build/main
```