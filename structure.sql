CREATE TABLE `session` (
  `sessId` int(11) UNSIGNED NOT NULL,
  `sockfd` int(11) NOT NULL,
  `host` varchar(15) NOT NULL,
  `port` mediumint(5) UNSIGNED NOT NULL,
  `tid` int(11) UNSIGNED NOT NULL,
  `dateline` int(11) NOT NULL,
  `createTime` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `uid` int(10) UNSIGNED NOT NULL,
  `loginDateline` int(11) NOT NULL,
  `loginTime` timestamp NOT NULL DEFAULT '0000-00-00 00:00:00',
  `logoutDateline` int(11) NOT NULL,
  `logoutTime` timestamp NOT NULL DEFAULT '0000-00-00 00:00:00'
) ENGINE=MEMORY DEFAULT CHARSET=utf8;
