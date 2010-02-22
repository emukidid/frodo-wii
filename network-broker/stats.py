import sys, pickle, time
from operator import itemgetter

class Container:
    def __init__(self):
        self.total_connections = 0
        self.country_count = {}
        self.last_10 = []

    def copy_from_other(self, other):
        try:
            self.total_connections = other.total_connections
            self.country_count = other.country_count
            self.last_10 = other.last_10
        except:
            pass

    def add_country(self, country):
        try:
            cur = self.country_count[country]
        except KeyError, e:
            cur = 0

        self.country_count[country] = cur + 1

    def add_connection(self, who, country):
        time_now = time.strftime("%Y-%m-%d %H:%M", time.gmtime())
        s = "%s - %s (%s)" % (time_now, who, country)
        self.total_connections = self.total_connections + 1
        self.last_10 = [s] + self.last_10[:9]
        self.add_country(country)

class HtmlGenerator:
    def __init__(self, container):
        self.container = container

    def generate(self, outf):
        sorted_countries = sorted(self.container.country_count.items(),
                                  reverse=True, key=itemgetter(1))

        outf.write("<html><body>\n")
        outf.write("<H2>Frodo-Wii network statistics</H2>\n")
        outf.write("The total number of connections is <b>%d</b><br><br>\n" % (self.container.total_connections))
        outf.write("<TABLE border=\"0\" cellpadding=\"0\">\n")
        outf.write("<TR><TD><H3>Last %d connections</H3></TD><TD>&nbsp;</TD<TD colspan=4><H3>Random connection screenshots</TD></TR>\n" %
                   (len(self.container.last_10)) )

        cnt = 0
        for item in self.container.last_10:
            images = ""
            if cnt % 4 == 0:
                cur = cnt
                images = "<TH ROWSPAN=4><IMG SRC=\"images/%d.png\"></TH><TH ROWSPAN=4><IMG SRC=\"images/%d.png\"></TH><TH ROWSPAN=4><IMG SRC=\"images/%d.png\"></TH><TH ROWSPAN=4><IMG SRC=\"images/%d.png\"></TH>" % (cur, cur + 1, cur + 2, cur + 3)
            outf.write("<TR><TD>%s</TD><TD>&nbsp;</TD>%s</TR>\n" % (item, images) )
            cnt = cnt + 1
        outf.write("<TR><TD>&nbsp;</TD></TR>")
        outf.write("</TABLE>\n")

        outf.write("<br><br><TABLE border=\"0\" cellpadding=\"0\">\n")
        outf.write("<TR><TD colspan=3><H3>List of countries</H3></TD></TR>\n")
        count = 1

        n_countries = len(sorted_countries)
        for i in range(0, n_countries / 3):
            c1, n1 = sorted_countries[i]
            c2, n2 = sorted_countries[i + n_countries / 3]
            c3, n3 = sorted_countries[i + (n_countries / 3) * 2]
            outf.write("<TR><TD><b>%3d</b></TD><TD>&nbsp;</TD><TD>%s (%d)</TD><TD>&nbsp;</TD><TD>&nbsp;</TD><TD>&nbsp;</TD><TD><b>%3d</b></TD><TD>&nbsp;</TD><TD>%s (%d)</TD><TD>&nbsp;</TD><TD>&nbsp;</TD><TD>&nbsp;</TD><TD><b>%3d</b></TD><TD>&nbsp;</TD><TD>%s (%d)</TD></TR>\n" %
                       (count, c1, n1, count + n_countries / 3, c2, n2, count + (n_countries / 3) * 2, c3, n3) )
            count = count + 1
        # Output the last if it's odd
        for i in range(0, n_countries % 3):
            c1, n1 = sorted_countries[n_countries - i - 1]
            outf.write("<TR><TD></TD><TD></TD><TD></TD><TD></TD><TD></TD><TD></TD><TD></TD><TD></TD><TD></TD><TD></TD><TD></TD><TD></TD><TD><b>%3d</b></TD><TD>&nbsp;</TD><TD>%s (%d)</TD></TR>\n" %
                       (n_countries - n_countries % 3 + i + 1, c1, n1) )
            
            
        outf.write("</TABLE>\n")
        outf.write("</body></html>\n")


g_stat = None
def save(filename):
    global g_stat

    of = open(filename, "wb")
    pickle.dump(g_stat, of)
    of.close()

def generate_html(filename):
    of = open(filename, "wb")
    hg = HtmlGenerator(g_stat)
    hg.generate(of)
    of.close()


def load(filename):
    global g_stat

    g_stat = Container()
    try:
        of = open(filename, "r")
        other = pickle.load(of)
        g_stat.copy_from_other(other)
        of.close()
    except:
        pass

def add_connection(who, country):
    g_stat.add_connection(who, country)

if __name__ == "__main__":
    load("/tmp/vobb")
    for i in range(0, 10):
        add_connection("MABOO", "Unknown country")
    add_connection("SIMONK", "Sweden")
    add_connection("SIMONK", "Sweden")
    add_connection("SIMONK", "Sweden")
    add_connection("Linda", "Germany")
    add_connection("Linda", "Germany")
    save("/tmp/vobb")

    hg = HtmlGenerator(g_stat)
    hg.generate(sys.stdout)
