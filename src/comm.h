# define INBUF_SIZE	2048
# define OUTBUF_SIZE	8192
# define BINBUF_SIZE	8192


extern bool		    conn_init	 P((int, unsigned int, unsigned int));
extern void		    conn_finish	 P((void));
extern void		    conn_listen	 P((void));
extern struct _connection_ *conn_tnew	 P((void));
extern struct _connection_ *conn_bnew	 P((void));
extern void		    conn_udp	 P((struct _connection_*));
extern void		    conn_del	 P((struct _connection_*));
extern void		    conn_block	 P((struct _connection_*, int));
extern int		    conn_select	 P((Uint, unsigned int));
extern int		    conn_read	 P((struct _connection_*, char*,
					    unsigned int));
extern int		    conn_udpread P((struct _connection_*, char*,
					    unsigned int));
extern int		    conn_write	 P((struct _connection_*, char*,
					    unsigned int));
extern int		    conn_udpwrite P((struct _connection_*, char*,
					     unsigned int));
extern bool		    conn_wrdone	 P((struct _connection_*));
extern char		   *conn_ipnum	 P((struct _connection_*));
extern char		   *conn_ipname	 P((struct _connection_*));

extern bool	comm_init	P((int, unsigned int, unsigned int));
extern void	comm_finish	P((void));
extern void	comm_listen	P((void));
extern int	comm_send	P((object*, string*));
extern int	comm_udpsend	P((object*, string*));
extern bool	comm_echo	P((object*, int));
extern void	comm_flush	P((void));
extern void	comm_block	P((object*, int));
extern void	comm_receive	P((frame*, Uint, unsigned int));
extern string  *comm_ip_number	P((lpcenv*, object*));
extern string  *comm_ip_name	P((lpcenv*, object*));
extern void	comm_close	P((frame*, object*));
extern array   *comm_users	P((dataspace*));
