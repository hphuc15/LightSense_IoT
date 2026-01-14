# Remote Client Access

Some MariaDB packages bind MariaDB to `127.0.0.1` (the loopback IP address) by default as a security measure using the [bind-address](https://mariadb.com/docs/server/server-management/variables-and-modes/server-system-variables#bind_address) configuration directive. Old MySQL packages sometimes disabled TCP/IP networking altogether using the [skip-networking](https://mariadb.com/docs/server/server-management/variables-and-modes/server-system-variables#skip_networking) directive. Before going in to how to configure these, let's explain what each of them actually does:

- [skip-networking](https://mariadb.com/docs/server/server-management/variables-and-modes/server-system-variables#skip_networking) is fairly simple. It just tells MariaDB to run without any of the TCP/IP networking options.
- [bind-address](https://mariadb.com/docs/server/server-management/variables-and-modes/server-system-variables#bind_address) requires a little bit of background information. A given server usually has at least two networking interfaces (although this is not required) and can easily have more. The two most common are a Loopback network device and a physical Network Interface Card (NIC) which allows you to communicate with the network. MariaDB is bound to the loopback interface by default because it makes it impossible to connect to the TCP port on the server from a remote host (the bind-address must refer to a local IP address, or you will receive a fatal error and MariaDB will not start). This of course is not desirable if you want to use the TCP port from a remote host, so you must remove this bind-address directive or replace it either `0.0.0.0` to listen on all interfaces, or the address of a specific public interface.

{% tabs %}
{% tab title="Current" %}
Multiple comma-separated addresses can be given to `bind_address` to allow the server to listen on more than one specific interface while not listening on others.

If [bind-address](https://mariadb.com/docs/server/server-management/variables-and-modes/server-system-variables#bind_address) is bound to 127.0.0.1 (localhost), one can't connect to the MariaDB server from other hosts or from the same host over TCP/IP on a different interface than the loopback (127.0.0.1). This for example will not work (connecting with a hostname that points to a local IP of the host):

```bash
(/my/maria-10.11) ./client/mariadb --host=myhost --protocol=tcp --port=3306 test
ERROR 2002 (HY000): Can't connect to MySQL server on 'myhost' (115)
(/my/maria-10.11) telnet myhost 3306
Trying 192.168.0.11...
telnet: connect to address 192.168.0.11: Connection refused
```

Using 'localhost' works when binding with `bind_address`:

```bash
(my/maria-10.11) ./client/mariadb --host=localhost --protocol=tcp --port=3306 test
Reading table information for completion of table and column names
You can turn off this feature to get a quicker startup with -A

Welcome to the MariaDB monitor.  Commands end with ; or \g.
...
```

{% endtab %}

{% tab title="< 10.11" %}
Multiple comma-separated addresses **cannot** be given to `bind_address` . Use a single address.
{% endtab %}
{% endtabs %}

## Finding the Defaults File

To enable MariaDB to listen to remote connections, you need to edit your defaults file. See [Configuring MariaDB with my.cnf](https://mariadb.com/docs/server/server-management/install-and-upgrade-mariadb/configuring-mariadb/configuring-mariadb-with-option-files) for more detail.

Common locations for defaults files:

```bash
* /etc/my.cnf                              (*nix/BSD)
  * $MYSQL_HOME/my.cnf                       (*nix/BSD) *Most Notably /etc/mysql/my.cnf
  * SYSCONFDIR/my.cnf                        (*nix/BSD)
  * DATADIR\my.ini                           (Windows)
```

You can see which defaults files are read and in which order by executing:

```bash
shell> mariadbd --help --verbose
mariadbd  Ver 10.11.5-MariaDB for linux-systemd on x86_64 (MariaDB Server)
Copyright (c) 2000, 2018, Oracle, MariaDB Corporation Ab and others.

Starts the MariaDB database server.

Usage: ./mariadbd [OPTIONS]

Default options are read from the following files in the given order:
/etc/my.cnf /etc/mysql/my.cnf ~/.my.cnf
```

The last line shows which defaults files are read.

## Editing the Defaults File

Once you have located the defaults file, use a text editor to open the file and try to find lines like this under the `[mysqld]` section:

```ini
[mysqld]
    ...
    skip-networking
    ...
    bind-address = <some ip-address>
    ...
```

The lines may not be in this particular order, but the order doesn't matter.

If you are able to locate these lines, make sure they are both commented out (prefaced with hash (#) characters), so that they look like this:

```ini
[mysqld]
    ...
    #skip-networking
    ...
    #bind-address = <some ip-address>
    ...
```

Again, the order of these lines don't matter.

Alternatively, just add the following lines **at the end** of your `.my.cnf` (notice that the file name starts with a dot) file in your home directory or alternative **last** in your `/etc/my.cnf` file.

```ini
[mysqld]
skip-networking=0
skip-bind-address
```

This works as one can have any number of \[mysqld] sections.

Save the file and restart the mariadbd daemon or service (see [Starting and Stopping MariaDB](https://mariadb.com/docs/server/server-management/starting-and-stopping-mariadb)).

You can check the options mariadbd is using by executing:

```bash
shell> ./sql/mariadbd --print-defaults
./sql/mariadbd would have been started with the following arguments:
--bind-address=127.0.0.1 --innodb_file_per_table=ON --server-id=1 --skip-bind-address ...
```

It doesn't matter if you have the original `--bind-address` left as the later `--skip-bind-address` will overwrite it.

## Granting User Connections From Remote Hosts

Now that your MariaDB server installation is setup to accept connections from remote hosts, we have to add a user that is allowed to connect from something other than 'localhost' (Users in MariaDB are defined as 'user'@'host', so '`chadmaynard'@'localhost`' and '`chadmaynard'@'1.1.1.1`' (or 'chadmaynard'@'server.domain.local') are different users that can have different permissions and/or passwords.

To create a new user:

- Log into the [mariadb command line client](https://mariadb.com/docs/server/clients-and-utilities/mariadb-client/mariadb-command-line-client) (or your favorite graphical client if you wish):

```bash
Welcome to the MariaDB monitor.  Commands end with ; or \g.
Your MariaDB connection id is 36
Server version: 5.5.28-MariaDB-mariadb1~lucid mariadb.org binary distribution

Copyright (c) 2000, 2012, Oracle, Monty Program Ab and others.

Type 'help;' or '\h' for help. Type '\c' to clear the current input statement.

MariaDB [(none)]>
```

- if you are interested in viewing any existing remote users, issue the following SQL statement on the [mysql.user](https://mariadb.com/docs/server/reference/system-tables/the-mysql-database-tables/mysql-user-table) table:

```sql
SELECT User, Host FROM mysql.user WHERE Host <> 'localhost';
+--------+-----------+
| User   | Host      |
+--------+-----------+
| daniel | %         |
| root   | 127.0.0.1 |
| root   | ::1       |
| root   | gandalf   |
+--------+-----------+
4 rows in set (0.00 sec)
```

(If you have a fresh install, it is normal for no rows to be returned)

Now you have some decisions to make. At the heart of every grant statement you have these things:

- list of allowed privileges
- what database/tables these privileges apply to
- username
- host this user can connect from
- and optionally a password

It is common for people to want to create a "root" user that can connect from anywhere, so as an example, we'll do just that, but to improve on it we'll create a root user that can connect from anywhere on my local area network (LAN), which has addresses in the subnet `192.168.100.0/24`. This is an improvement because opening a MariaDB server up to the Internet and granting access to all hosts is bad practice.

```sql
GRANT ALL PRIVILEGES ON *.* TO 'root'@'192.168.100.%'
  IDENTIFIED BY 'my-new-password' WITH GRANT OPTION;
```

`%` is a wildcard.

For more information about how to use GRANT, please see the [GRANT](https://mariadb.com/docs/server/reference/sql-statements/account-management-sql-statements/grant) page.

At this point, we have accomplished our goal and we have a user 'root' that can connect from anywhere on the `192.168.100.0/24` LAN.

## Port 3306 is Configured in Firewall

One more point to consider whether the firewall is configured to allow incoming request from remote clients:

On RHEL and CentOS 7, it may be necessary to configure the firewall to allow TCP access to MariaDB from remote hosts. To do so, execute both of these commands:

```bash
firewall-cmd --add-port=3306/tcp
firewall-cmd --permanent --add-port=3306/tcp
```

## Caveats

- If your system is running a software firewall (or behind a hardware firewall or NAT) you must allow connections destined to TCP port that MariaDB runs on (by default and almost always 3306).
- To undo this change and not allow remote access anymore, simply remove the `skip-bind-address` line or uncomment the [bind-address](https://mariadb.com/docs/server/server-management/variables-and-modes/server-system-variables#bind_address) line in your defaults file. The end result should be that you should have in the output from `./sql/mariadbd --print-defaults` the option `--bind-address=127.0.0.1` and no `--skip-bind-address`.

_The initial version of this article was copied, with permission, from_ [_Remote_Clients_Cannot_Connect_](https://hashmysql.org/wiki/Remote_Clients_Cannot_Connect) _on 2012-10-30._

<sub>_This page is licensed: CC BY-SA / Gnu FDL_</sub>

---

# Truy cập Client từ xa (Bản dịch tiếng Việt)

Một số gói MariaDB ràng buộc MariaDB với `127.0.0.1` (địa chỉ IP loopback) theo mặc định như một biện pháp bảo mật sử dụng chỉ thị cấu hình [bind-address](https://mariadb.com/docs/server/server-management/variables-and-modes/server-system-variables#bind_address). Các gói MySQL cũ đôi khi vô hiệu hóa hoàn toàn mạng TCP/IP bằng cách sử dụng chỉ thị [skip-networking](https://mariadb.com/docs/server/server-management/variables-and-modes/server-system-variables#skip_networking). Trước khi đi vào cách cấu hình chúng, hãy giải thích ý nghĩa của từng chỉ thị:

- [skip-networking](https://mariadb.com/docs/server/server-management/variables-and-modes/server-system-variables#skip_networking) khá đơn giản. Nó chỉ yêu cầu MariaDB chạy mà không có bất kỳ tùy chọn mạng TCP/IP nào.
- [bind-address](https://mariadb.com/docs/server/server-management/variables-and-modes/server-system-variables#bind_address) cần một chút thông tin nền. Một máy chủ thường có ít nhất hai giao diện mạng (mặc dù điều này không bắt buộc) và có thể dễ dàng có nhiều hơn. Hai loại phổ biến nhất là thiết bị mạng Loopback và Card giao diện mạng vật lý (NIC) cho phép bạn giao tiếp với mạng. MariaDB được ràng buộc với giao diện loopback theo mặc định vì điều này khiến không thể kết nối tới cổng TCP trên máy chủ từ một máy chủ từ xa (bind-address phải tham chiếu đến một địa chỉ IP cục bộ, nếu không bạn sẽ nhận được lỗi nghiêm trọng và MariaDB sẽ không khởi động). Tất nhiên điều này không mong muốn nếu bạn muốn sử dụng cổng TCP từ máy chủ từ xa, vì vậy bạn phải xóa chỉ thị bind-address này hoặc thay thế nó bằng `0.0.0.0` để lắng nghe trên tất cả các giao diện, hoặc địa chỉ của một giao diện công khai cụ thể.

{% tabs %}
{% tab title="Phiên bản hiện tại" %}
Nhiều địa chỉ được phân tách bằng dấu phẩy có thể được cung cấp cho `bind_address` để cho phép máy chủ lắng nghe trên nhiều giao diện cụ thể trong khi không lắng nghe trên các giao diện khác.

Nếu [bind-address](https://mariadb.com/docs/server/server-management/variables-and-modes/server-system-variables#bind_address) được ràng buộc với 127.0.0.1 (localhost), người dùng không thể kết nối tới máy chủ MariaDB từ các máy chủ khác hoặc từ cùng một máy chủ qua TCP/IP trên một giao diện khác với loopback (127.0.0.1). Ví dụ, điều này sẽ không hoạt động (kết nối với tên máy chủ trỏ đến IP cục bộ của máy chủ):

```bash
(/my/maria-10.11) ./client/mariadb --host=myhost --protocol=tcp --port=3306 test
ERROR 2002 (HY000): Can't connect to MySQL server on 'myhost' (115)
(/my/maria-10.11) telnet myhost 3306
Trying 192.168.0.11...
telnet: connect to address 192.168.0.11: Connection refused
```

Sử dụng 'localhost' hoạt động khi ràng buộc với `bind_address`:

```bash
(my/maria-10.11) ./client/mariadb --host=localhost --protocol=tcp --port=3306 test
Reading table information for completion of table and column names
You can turn off this feature to get a quicker startup with -A

Welcome to the MariaDB monitor.  Commands end with ; or \g.
...
```

{% endtab %}

{% tab title="< 10.11" %}
Nhiều địa chỉ được phân tách bằng dấu phẩy **không thể** được cung cấp cho `bind_address`. Sử dụng một địa chỉ duy nhất.
{% endtab %}
{% endtabs %}

## Tìm tệp cấu hình mặc định

Để cho phép MariaDB lắng nghe các kết nối từ xa, bạn cần chỉnh sửa tệp cấu hình mặc định. Xem [Cấu hình MariaDB với my.cnf](https://mariadb.com/docs/server/server-management/install-and-upgrade-mariadb/configuring-mariadb/configuring-mariadb-with-option-files) để biết thêm chi tiết.

Các vị trí phổ biến cho tệp cấu hình mặc định:

```bash
* /etc/my.cnf                              (*nix/BSD)
  * $MYSQL_HOME/my.cnf                       (*nix/BSD) *Đặc biệt là /etc/mysql/my.cnf
  * SYSCONFDIR/my.cnf                        (*nix/BSD)
  * DATADIR\my.ini                           (Windows)
```

Bạn có thể xem tệp cấu hình mặc định nào được đọc và theo thứ tự nào bằng cách thực thi:

```bash
shell> mariadbd --help --verbose
mariadbd  Ver 10.11.5-MariaDB for linux-systemd on x86_64 (MariaDB Server)
Copyright (c) 2000, 2018, Oracle, MariaDB Corporation Ab and others.

Starts the MariaDB database server.

Usage: ./mariadbd [OPTIONS]

Default options are read from the following files in the given order:
/etc/my.cnf /etc/mysql/my.cnf ~/.my.cnf
```

Dòng cuối cùng hiển thị các tệp cấu hình mặc định được đọc.

## Chỉnh sửa tệp cấu hình mặc định

Sau khi bạn đã xác định được tệp cấu hình mặc định, sử dụng trình soạn thảo văn bản để mở tệp và cố gắng tìm các dòng như thế này trong phần `[mysqld]`:

```ini
[mysqld]
    ...
    skip-networking
    ...
    bind-address = <some ip-address>
    ...
```

Các dòng có thể không theo thứ tự cụ thể này, nhưng thứ tự không quan trọng.

Nếu bạn có thể xác định được các dòng này, hãy đảm bảo chúng đều được chú thích (có ký tự thăng (#) ở đầu), để chúng trông như thế này:

```ini
[mysqld]
    ...
    #skip-networking
    ...
    #bind-address = <some ip-address>
    ...
```

Một lần nữa, thứ tự của các dòng này không quan trọng.

Ngoài ra, chỉ cần thêm các dòng sau **ở cuối** tệp `.my.cnf` (lưu ý rằng tên tệp bắt đầu bằng dấu chấm) trong thư mục home của bạn hoặc thay thế **cuối cùng** trong tệp `/etc/my.cnf` của bạn.

```ini
[mysqld]
skip-networking=0
skip-bind-address
```

Điều này hoạt động vì bạn có thể có bất kỳ số lượng phần \[mysqld] nào.

Lưu tệp và khởi động lại daemon hoặc dịch vụ mariadbd (xem [Khởi động và Dừng MariaDB](https://mariadb.com/docs/server/server-management/starting-and-stopping-mariadb)).

Bạn có thể kiểm tra các tùy chọn mà mariadbd đang sử dụng bằng cách thực thi:

```bash
shell> ./sql/mariadbd --print-defaults
./sql/mariadbd would have been started with the following arguments:
--bind-address=127.0.0.1 --innodb_file_per_table=ON --server-id=1 --skip-bind-address ...
```

Không quan trọng nếu bạn vẫn còn `--bind-address` ban đầu vì `--skip-bind-address` sau này sẽ ghi đè nó.

## Cấp quyền kết nối cho người dùng từ các máy chủ từ xa

Bây giờ cài đặt máy chủ MariaDB của bạn đã được thiết lập để chấp nhận kết nối từ các máy chủ từ xa, chúng ta phải thêm người dùng được phép kết nối từ địa chỉ khác 'localhost' (Người dùng trong MariaDB được định nghĩa là 'user'@'host', vì vậy '`chadmaynard'@'localhost`' và '`chadmaynard'@'1.1.1.1`' (hoặc 'chadmaynard'@'server.domain.local') là những người dùng khác nhau có thể có các quyền và/hoặc mật khẩu khác nhau.

Để tạo người dùng mới:

- Đăng nhập vào [mariadb command line client](https://mariadb.com/docs/server/clients-and-utilities/mariadb-client/mariadb-command-line-client) (hoặc ứng dụng đồ họa yêu thích của bạn nếu muốn):

```bash
Welcome to the MariaDB monitor.  Commands end with ; or \g.
Your MariaDB connection id is 36
Server version: 5.5.28-MariaDB-mariadb1~lucid mariadb.org binary distribution

Copyright (c) 2000, 2012, Oracle, Monty Program Ab and others.

Type 'help;' or '\h' for help. Type '\c' to clear the current input statement.

MariaDB [(none)]>
```

- nếu bạn muốn xem bất kỳ người dùng từ xa hiện có nào, hãy thực thi câu lệnh SQL sau trên bảng [mysql.user](https://mariadb.com/docs/server/reference/system-tables/the-mysql-database-tables/mysql-user-table):

```sql
SELECT User, Host FROM mysql.user WHERE Host <> 'localhost';
+--------+-----------+
| User   | Host      |
+--------+-----------+
| daniel | %         |
| root   | 127.0.0.1 |
| root   | ::1       |
| root   | gandalf   |
+--------+-----------+
4 rows in set (0.00 sec)
```

(Nếu bạn có cài đặt mới, không có hàng nào được trả về là bình thường)

Bây giờ bạn có một số quyết định cần đưa ra. Cốt lõi của mọi câu lệnh grant bạn có những thứ sau:

- danh sách các đặc quyền được phép
- cơ sở dữ liệu/bảng nào mà các đặc quyền này áp dụng
- tên người dùng
- máy chủ mà người dùng này có thể kết nối từ đó
- và tùy chọn là mật khẩu

Mọi người thường muốn tạo một người dùng "root" có thể kết nối từ bất kỳ đâu, vì vậy làm ví dụ, chúng ta sẽ làm điều đó, nhưng để cải thiện nó, chúng ta sẽ tạo một người dùng root có thể kết nối từ bất kỳ đâu trên mạng cục bộ (LAN) của tôi, có địa chỉ trong mạng con `192.168.100.0/24`. Đây là một cải tiến vì việc mở một máy chủ MariaDB ra Internet và cấp quyền truy cập cho tất cả các máy chủ là một thực hành tồi.

```sql
GRANT ALL PRIVILEGES ON *.* TO 'root'@'192.168.100.%'
  IDENTIFIED BY 'my-new-password' WITH GRANT OPTION;
```

`%` là một ký tự đại diện.

Để biết thêm thông tin về cách sử dụng GRANT, vui lòng xem trang [GRANT](https://mariadb.com/docs/server/reference/sql-statements/account-management-sql-statements/grant).

Tại thời điểm này, chúng ta đã đạt được mục tiêu và chúng ta có một người dùng 'root' có thể kết nối từ bất kỳ đâu trên LAN `192.168.100.0/24`.

## Cổng 3306 được cấu hình trong Tường lửa

Một điểm khác cần xem xét là liệu tường lửa có được cấu hình để cho phép yêu cầu đến từ các client từ xa hay không:

Trên RHEL và CentOS 7, có thể cần cấu hình tường lửa để cho phép truy cập TCP tới MariaDB từ các máy chủ từ xa. Để làm điều này, hãy thực thi cả hai lệnh sau:

```bash
firewall-cmd --add-port=3306/tcp
firewall-cmd --permanent --add-port=3306/tcp
```

## Lưu ý

- Nếu hệ thống của bạn đang chạy tường lửa phần mềm (hoặc đằng sau tường lửa phần cứng hoặc NAT), bạn phải cho phép các kết nối đến cổng TCP mà MariaDB chạy trên đó (theo mặc định và gần như luôn luôn là 3306).
- Để hoàn tác thay đổi này và không cho phép truy cập từ xa nữa, chỉ cần xóa dòng `skip-bind-address` hoặc bỏ chú thích dòng [bind-address](https://mariadb.com/docs/server/server-management/variables-and-modes/server-system-variables#bind_address) trong tệp cấu hình mặc định của bạn. Kết quả cuối cùng là bạn nên có trong đầu ra từ `./sql/mariadbd --print-defaults` tùy chọn `--bind-address=127.0.0.1` và không có `--skip-bind-address`.

_Phiên bản ban đầu của bài viết này được sao chép, với sự cho phép, từ_ [_Remote_Clients_Cannot_Connect_](https://hashmysql.org/wiki/Remote_Clients_Cannot_Connect) _vào ngày 30-10-2012._

<sub>_Trang này được cấp phép theo: CC BY-SA / Gnu FDL_</sub>

{% @marketo/form formId="4316" %}

**Source:** [Remote Client Access | MariaDB Documentation](https://mariadb.com/kb/en/configuring-mariadb-for-remote-client-access/)
