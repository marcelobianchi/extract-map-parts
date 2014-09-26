/*
    This file is part of extract-map-parts package.

    extract-map-parts is a tool to managing and do simple processing with XY
    segment files normally used by the GMT program.
    Copyright (C) 2014  Marcelo B. de Bianchi

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#include <stdlib.h> 
#include <libgen.h>
#include <math.h>
#include <ctype.h>
#include <string.h>
#include <cpgplot.h>
#include <X11/Xlib.h>

#define USEL 0
#define NSEL 1
#define  SEL 2

void resizemax(float scale, float ratio)
{
    Display *disp;
    int X, Y;

    /* Xlib code */
    disp = XOpenDisplay(NULL);
    if (disp == NULL) {
        fprintf(stderr, "No Display.\n");
        return;
    } else {
        Y = XDisplayHeightMM(disp, 0);
        X = XDisplayWidthMM(disp, 0);
    }
    XCloseDisplay(disp);
    /* End of Xlib code */

    /*
     * Find the size that fits the window with a certain ratio
     */
    float width;
    if (ratio < 0)
        ratio = (float) Y / (float) X;

    width = X * scale;
    if ((width*ratio) > Y)
        width = (Y / ratio) * scale;

    cpgpap(width / 25.4, ratio);
    cpgpage();
}

int opengr()
{
    char aux[8];
    int i = 5;
    int GRid;

    cpgqinf("STATE", aux, &i);
    if (strncmp(aux, "OPEN", i) != 0) {
        GRid = cpgopen("/xwindow");
        cpgask(0);
        return (GRid);
    } else {
        fprintf(stderr, "Device alredy open !!");
        return -1;
    }
}

long getpointsinside(long ninp,float *x,float *y,long *l,long *selected_lines,float xt,float yt,float xb,float yb)
{
    long i;

    for(i=0;i<ninp;i++)
    {
        if (x[i]<=xb && x[i]>=xt)
            if ( y[i]<=yt && y[i]>=yb)
                selected_lines[l[i]]=SEL;
    }
    return 0;
}

void warn(char *message) {
    char ch;
    float ax,ay;
    cpgsvp(0.15,0.85,0.15,0.30);
    cpgswin(0.0,1.0,0.0,1.0);
    cpgrect(0.0,1.0,0.0,1.0);
    cpgsci(0);
    cpgrect(0.01,.99,0.01,0.99);
    cpgsci(1);
    cpgmtxt("T",-1.0,0.02,.0,message);
    cpgband (7, 0, 0, 0, &ax, &ay, &ch);
}

char *getfilename()
{
    char *filename;
    char *p,ch;
    float ax,ay;

    filename=malloc(128*sizeof(char));
    p=&filename[0];
    *p='\0';
    while(ch!=10)
    {
        cpgsvp(0.25,0.75,0.35,0.5);
        cpgswin(0.0,1.0,0.0,1.0);
        cpgrect(0.0,1.0,0.0,1.0);
        cpgsci(0);
        cpgrect(0.01,.99,0.01,0.99);
        cpgsci(1);
        cpgmtxt("T",-1.0,0.02,.0,"Enter filename to export:");
        cpgmtxt("T",-2.0,0.02,.0,filename);
        cpgband (7, 0, 0, 0, &ax, &ay, &ch);
        switch(ch)
        {
        case(13):
            *p='\0';
            return filename;
            break;

        case(8):
            p--;
            *p='\0';
            break;

        default:
            *p=ch;
            p++;
            *p='\0';
            break;
        }
    }

    return NULL;
}

long saveundo(long ninp, float *x, float *y, long *l, float **nx, float **ny, long **nl) {
    (*nx) = (float *) realloc((*nx), sizeof(float)*(ninp));
    (*ny) = (float *) realloc((*ny), sizeof(float)*(ninp));
    (*nl) = (long *)  realloc((*nl), sizeof(long)*(ninp));

    memcpy(*nx, x, sizeof(float) * ninp);
    memcpy(*ny, y, sizeof(float) * ninp);
    memcpy(*nl, l, sizeof(long) * ninp);

    return ninp;
}

long exportline(long ninp,float *x, float *y, long *l, long *selected_lines)
{
    long i, pl, nexp=0;
    FILE *out;
    char *filename=NULL;

    if (ninp == 0) return;

    filename=getfilename();
    out=fopen(filename,"w");
    if (out == NULL )
    {
        warn("Export Failed !");
        return -1;
    }

    i=0;
    pl=0;
    if (selected_lines[l[i]] == SEL)
    {
        nexp++;
        pl=1;
        fprintf(out,"> Line %d\n",l[i]);
    }
    if (pl) fprintf(out,"%f %f\n",x[i],y[i]);

    for (i=1;i<ninp;i++)
    {
        if (l[i]!=l[i-1])
        {
            pl=0;
            if (selected_lines[l[i]]==SEL)
            {
                nexp++;
                pl=1;
                fprintf(out,"> Line %d\n",l[i]);
            }
        }

        if (pl) fprintf(out,"%f %f\n",x[i],y[i]);
    }

    fprintf(stderr, "Exported %d lines to file %s\n",nexp,filename);
    if (filename!=NULL) free(filename);
    filename=NULL;
    fclose(out);
    return 0;
}

void reverse_line(long ninp,float *x,float *y,long *l, long line)
{
    long n;
    float *xt, *yt;
    long i,j, pt, pti=-999;

    for(i=0,n=0;i<ninp;i++)
    {
        if (l[i]==line)
        {
            n++;
            pt=i;
        }
        if (l[i]==line && pti==-999) pti=i;
    }

    xt=malloc(sizeof(float)*n);
    yt=malloc(sizeof(float)*n);

    if (xt!=NULL && yt!=NULL)
    {
        for(i=0;i<n;i++)
        {
            xt[i]=x[pt-i];
            yt[i]=y[pt-i];
        }

        for(i=0;i<n;i++)
        {
            x[pti+i]=xt[i];
            y[pti+i]=yt[i];
        }
    }

    free(xt);
    xt=NULL;
    free(yt);
    yt=NULL;
}

long findindex(float ax,float ay,float *x,float *y, long *l, long ninp)
{
    float dmin,di;
    long pmin;
    long i;

    if (ninp == 0) return -1;

    dmin=sqrt(powf(ax-x[0],2)+powf(ay-y[0],2));
    pmin=0;

    for(i=1;i<ninp;i++)
    {
        di=sqrt(powf(ax-x[i],2)+powf(ay-y[i],2));
        if (di<dmin)
        {
            dmin=di;
            pmin=i;
        }
    }

    return pmin;
}

long findline(float ax,float ay,float *x,float *y, long *l, long ninp)
{
    float dmin,di;
    long pmin;
    long i;

    if (ninp == 0) return -1;

    dmin=sqrt(powf(ax-x[0],2)+powf(ay-y[0],2));
    pmin=l[0];

    for(i=1;i<ninp;i++)
    {
        di=sqrt(powf(ax-x[i],2)+powf(ay-y[i],2));
        if (di<dmin)
        {
            dmin=di;
            pmin=l[i];
        }
    }

    return pmin;
}

void findlineedges(long *l, long ninp, long target, long *s, long *e) {
    long i;
    *s = -1;
    *e = -1;

    for(i=0;i<ninp;i++) {
        if (l[i] == target) {
            if (*s == -1) *s = i;
        }
        if (*s == -1) continue;
        if (l[i] != target) return;
        *e = i;
    }

    return;
}

void minmax(float *xx, long npts,float *xmin,float *xmax)
{
    long i;

    (*xmin)=(*xmax)=0.0;
    if (npts == 0) return;
    if (xx == NULL) return;

    (*xmin)=(*xmax)=xx[0];
    for(i=1;i<npts;i++)
    {
        if (*xmax<xx[i]) *xmax=xx[i];
        if (*xmin>xx[i]) *xmin=xx[i];
    }
}

void dminmax(long *xx, long npts, long *xmin,long *xmax)
{
    long i;

    (*xmin)=(*xmax)=-1;
    if (npts == 0) return;
    if (xx == NULL) return;

    (*xmin)=(*xmax)=xx[0];
    for(i=1;i<npts;i++)
    {
        if (*xmax<xx[i]) *xmax=xx[i];
        if (*xmin>xx[i]) *xmin=xx[i];
    }
}

void reversexy(long ninp, float *x, float *y)
{
    long i;
    float aux;

    for (i=0;i<ninp;i++)
    {
        aux=x[i];
        x[i]=y[i];
        y[i]=aux;
    }
}

long plotme(long argc, char ** argv, long ninp,float *x,float *y, long *l,long *selected_lines, long drawpoints, long drawline, long drawse, long drawlabel, long saved, long drawall)
{
    long i;
    char string[1500];
    long a,b;

    if (drawlabel) {
        // Titles
        cpgsci(1);
        cpgsch(1.2);
        cpgmtxt("T",2.2,0.5,0.5,"Manage Line Segments from GMT-XY files");
        cpgsch(0.8);
        cpgmtxt("T",1,1.0,1.0,"Press 'h' for help");
        dminmax(l,ninp,&a,&b);
        sprintf(string, "%d points from %d segments loaded %s",ninp, b+1, (saved)?"":"*");
        cpgmtxt("T",1,0.0,0.0,string);

        // Bottom infos
        sprintf(string, "File%s loaded: ", (argc > 2)?"s":"");
        for(i = 1; i < argc; i ++) {
            char *basepath = NULL;
            char string2[500] = "";
            strcpy(string2,argv[i]);
            basepath = (char *) basename(string2);

            if (i == 1)
                if (i == (argc - 1))
                    sprintf(string, "%s(%s)",string, basepath);
                else
                    sprintf(string, "%s(%s",string, basepath);
            else if (i == (argc - 1))
                sprintf(string, "%s, %s)",string, basepath);
            else
                sprintf(string, "%s, %s",string, basepath);
        }
        cpgmtxt("B",3.2,0.0,0.0,string);
        if (drawse) {
            cpgmtxt("B",3.2,1.0,1.0,"Segment start - o");
            cpgmtxt("B",4.5,1.0,1.0,"Segment end - x");
        }

        sprintf(string,"%s %s %s",(drawpoints)?"Points":"-",(drawline)?"Lines":"-",(drawse)?"Marks":"-");
        cpgmtxt("R",1.2,0.0,0.0,string);
    }

    if ((ninp == 0)||(x == NULL)||(y == NULL)||(l == NULL)||(selected_lines == NULL)) {
        cpgsch(1.0);
        cpgsci(1);
        return;
    }

    if (drawpoints) {
        int first = 0;
        int last = 0;
        long line = -1;

        if (drawse)
            cpgsch(0.1);
        else
            cpgsch(0.55);

        // Plot points
        line = -1;
        for(i=0; i < ninp; i++) {
            if (l[i] != line) {
                line = l[i];
                cpgsci( selected_lines[line]);
                first = 1;
            }
            if (!drawall && selected_lines[line] != SEL) continue;
            if (drawse) {
                last = ((i == (ninp-1)) || (l[i] != l[i+1]));
                if (first) {
                    cpgsch(1.25);
                    cpgpt1(x[i],y[i],4);
                    cpgsch(0.1);
                    first = 0;
                } else if (last) {
                    cpgsch(1.75);
                    cpgpt1(x[i],y[i],5);
                    cpgsch(0.1);
                }
            } else {
                cpgpt1(x[i],y[i],0);
            }
        }
        cpgsch (1.0);
    }

    if (drawline) {
        long line = -1;
        float oldx;

        // Plot segments
        line = l[0];
        cpgmove(x[0],y[0]);
        oldx = x[0];
        if ( selected_lines[ line ] != NSEL && selected_lines[ line ] != SEL)
            fprintf(stderr,"OOPS %d %d", line, selected_lines[line]);
        cpgsci( selected_lines[ line ] );
        for(i=1; i < ninp; i++) {
            if (l[i] != line) {
                line = l[i];
                cpgsci( selected_lines[line] );
                cpgmove(x[i],y[i]);
                oldx = x[i];
                continue;
            }

            if (!drawall && selected_lines[line] != SEL) continue;

            if (fabs(oldx - x[i]) > 180) {
                cpgmove(x[i],y[i]);
                oldx = x[i];
            }

            cpgdraw(x[i],y[i]);
            oldx = x[i];
        }
        cpgsch (1.0);
    }

    // General Reset
    cpgsch(1.0);
    cpgsci(1);

    return 0;
}

long *reindex(long ninp, long *l, long **selected_lines, long *nsel, long keep) {
    long seq = 0;
    long current = -1;
    long i = 0, linen = 0;

    long *selcurrent = *selected_lines;

    if (ninp == 0) {
        if ((*selected_lines) != NULL) free(*selected_lines);
        *nsel = 0;
        return NULL;
    }

    // Find max number of lines
    current = l[0];
    linen = 1;
    for(i = 1; i < ninp; i++) {
        if (l[i] != current) linen++;
        current = l[i];
    }

    // Build a new vector
    long *stemp = (long *) malloc(sizeof(long) * linen);
    for(i = 0; i < linen; i++) stemp[i] = NSEL;

    // Re-build
    seq = 0;
    current = l[0];
    if ( keep && (selcurrent != NULL) && (current < *nsel) && (current >= 0) ) stemp[seq] = selcurrent[current];
    for(i=0;i<ninp;i++) {
        if (l[i] != current) {
            seq ++;
            current = l[i];
            if ( keep && (selcurrent != NULL) && (current < *nsel) && (current >= 0) ) stemp[seq] = selcurrent[current];
        }
        l[i] = seq;
    }

    if (*selected_lines != NULL) free(*selected_lines);
    *nsel = linen;

    return stemp;
}

long insert(long ninp, float **x, float **y, long **l, long pos, long amount) {

    if (amount < 0) return ninp;

    // Re-allocate
#ifdef DEBUG
fprintf(stderr, "START INSERT Re-aloocating !\n");
#endif

    (*x) = (float*) realloc( (*x), sizeof(float) * (ninp+amount));
    (*y) = (float*) realloc( (*y), sizeof(float) * (ninp+amount));
    (*l) = (long*)  realloc( (*l), sizeof(long)  * (ninp+amount));

#ifdef DEBUG
fprintf(stderr, "DONE!\n");
#endif

    // Shift
    float *xm = *x;
    float *ym = *y;
    long  *lm = *l;

#ifdef DEBUG
fprintf(stderr, "NINP: %ld POS: %ld AMOUNT: %ld\n",ninp, pos, amount);
fprintf(stderr, "START INSERT Move !\n");
#endif

    memmove(&xm[pos+amount],&xm[pos],sizeof(float) * (ninp-pos));
    memmove(&ym[pos+amount],&ym[pos],sizeof(float) * (ninp-pos));
    memmove(&lm[pos+amount],&lm[pos],sizeof(long)  * (ninp-pos));

#ifdef DEBUG
fprintf(stderr, "DONE!\n");
#endif

    long i;
    for(i=pos;i<(pos+amount);i++) {
        xm[i] = 0.0;
        ym[i] = 0.0;
        lm[i] = -1;
    }

    ninp += amount;

#ifdef DEBUG
fprintf(stderr, "NNINP: %ld POS: %ld AMOUNT: %ld\n",ninp, pos, amount);
#endif
    return ninp;
}

long cut(long ninp, float **x, float **y, long **l, long n1, long n2) {
    long amount = (ninp - n2 - 1);
    long left = ninp - (n2 - n1 + 1);

    float *xm = *x;
    float *ym = *y;
    long *lm = *l;

    if (left < 0) {
        fprintf(stderr,"Cannot cut, left < 0");
        return ninp;
    }

#ifdef DEBUG
    fprintf(stderr,"Start CUT MOVE\n");
#endif

    memmove(&xm[n1],&xm[n2+1],sizeof(float) * amount);
    memmove(&ym[n1],&ym[n2+1],sizeof(float) * amount);
    memmove(&lm[n1],&lm[n2+1],sizeof(long) * amount);

#ifdef DEBUG
    fprintf(stderr,"End CUT MOVE\n");
#endif

#ifdef DEBUG
    fprintf(stderr,"Start CUT REALLOC\n");
#endif

    *x = (float*) realloc(*x, sizeof(float) * left);
    *y = (float*) realloc(*y, sizeof(float) * left);
    *l = (long*) realloc(*l, sizeof(long) * left);

#ifdef DEBUG
    fprintf(stderr,"End CUT REALLOC\n");
#endif

    return left;
}

long deletesegments(long ninp, float **x, float **y, long **l, long *selected_lines, long nsel) {
    long i = 0;

    while (i < nsel) {
        if (selected_lines[i] == SEL) {
            long n1, n2;
            findlineedges(*l,ninp,i,&n1,&n2);
            ninp = cut(ninp,x,y,l,n1,n2);
            selected_lines[i] = NSEL;
            i = 0;
            continue;
        }
        i++;
    }

    return ninp;
}

long addsegment(long ninp, float **x, float **y, long **l) {
    float ax = 0.0,ay = 0.0;
    float fx,fy;
    char ch = 'A';
    long a,line;

    dminmax(*l,ninp,&a,&line);
    line++;

    cpgsch(0.8);
    cpgmtxt("B",-1.0,0.5,0.5,"Q or C to stop, C closes the polygon.");
    cpgsch(1.0);

    cpgband (7, 0, 0, 0, &ax, &ay, &ch);
    ch = toupper (ch);
    cpgmove(ax,ay);
    fx = ax;
    fy = ay;
    while ( ch!='Q' && ch !='C' ) {
        *x = (float *) realloc(*x, sizeof(float)*(ninp+1));
        *y = (float *) realloc(*y, sizeof(float)*(ninp+1));
        *l = (long *)   realloc(*l, sizeof(long)*(ninp+1));
        (*x)[ninp] = ax;
        (*y)[ninp] = ay;
        (*l)[ninp] = line;
        ninp++;
        cpgband (1, 1, ax, ay, &ax, &ay, &ch);
        cpgdraw(ax,ay);
        ch = toupper (ch);
    }

    // Last point
    *x = (float *) realloc(*x, sizeof(float)*(ninp+1));
    *y = (float *) realloc(*y, sizeof(float)*(ninp+1));
    *l = (long *)   realloc(*l, sizeof(long)*(ninp+1));
    if (ch == 'C') {
        (*x)[ninp] = fx;
        (*y)[ninp] = fy;
    } else {
        (*x)[ninp] = ax;
        (*y)[ninp] = ay;
    }
    (*l)[ninp] = line;
    ninp++;

    return ninp;
}

long importfile(char *filename,float **x,float **y,long **l, long ninp)
{
    FILE *in;
    long line = 0;
    long i = 0;
    long fileline = 0;
    char readline[500] = "";

    if ((in=fopen(filename,"r"))==NULL) {
        sprintf(readline,"Cannot open and read the give file %s",filename);
        warn(readline);
        return ninp;
    }

    // Restore the last state

    // Find the last line indexed
    dminmax(*l,ninp,&i,&line);

    // Configure the number of samples in the input line
    i = ninp;

    fgets(readline,499,in);
    fileline++;
    do {
        if (readline[0] == '>') {
            line++;
        } else {
            float xt = 0.0;
            float yt = 0.0;
            long r;

            r = sscanf(readline,"%f %f",&xt,&yt);

            if (r == 2) {
                *x = (float *) realloc(*x, sizeof(float)*(i+1));
                *y = (float *) realloc(*y, sizeof(float)*(i+1));
                *l = (long *) realloc(*l, sizeof(long)*(i+1));
                (*x)[i] = xt;
                (*y)[i] = yt;
                (*l)[i] = line;
                i++;
            } else {
                fprintf(stderr,"Error reading file at line %d\n%s\n",fileline,readline);
                return (-1);
            }
        }
        fgets(readline,499,in);
        fileline++;
    } while (!feof(in));

    fclose(in);
    return i;
}

long savefile(char *filename, long ninp, float *x,float *y,long *l) {
    FILE *in;
    long i,oldl;

    if ((in=fopen(filename,"w"))==NULL) {
        fprintf(stderr,"Cannot open file with write permission (%s)\n",filename);
        return -1;
    }

    for(i=0; i < ninp; i++) {
        if (l[i] != oldl)
            fprintf(in,">\n");
        fprintf(in,"%f %f\n",x[i],y[i]);
        oldl = l[i];
    }

    fclose(in);
    return 0;
}

void unwrap(long ninp, float *x){
    long i;
    for(i = 0; i < ninp; i++) {
        if (x[i] < 0 ) x[i] = 360 - fabs(x[i]);
    }
    return;
}

void wrap(long ninp, float *x) {
    long i;
    for(i = 0; i < ninp; i++) {
        if (x[i] > 180 ) {
            x[i] = fabs(x[i]) - 360;
        }
    }
    return;
}

void resetzoom(float *x, float *y, long ninp, float *xmin, float *xmax, float *ymin, float *ymax) {
    // Zoom all data
    float x1 = 0.0,x2 = 0.0;
    float y1 = 0.0,y2 = 0.0;

    minmax(x,ninp,&x1,&x2);
    minmax(y,ninp,&y1,&y2);

    x1 -= (x2-x1) * 0.05;
    x2 += (x2-x1) * 0.05;
    y1 -= (y2-y1) * 0.05;
    y2 += (y2-y1) * 0.05;

    if (x1 == x2) {
        x1 -= 0.5;
        x2 += 0.5;
    }

    if (y1 == y2) {
        y1 -= 0.5;
        y2 += 0.5;
    }

    *xmin = x1;
    *xmax = x2;
    *ymin = y1;
    *ymax = y2;

    return;
}

void zoom(float *x, float *y, long ninp, float x1, float y1, float *xmin, float *xmax, float *ymin, float *ymax) {
    float temp;
    float x2;
    float y2;
    char ch;

    cpgband (2, 0, x1, y1, &x2, &y2, &ch);

    if ( (x1 == x2) && (y1 == y2) )
        return resetzoom(x,y,ninp,xmin,xmax,ymin,ymax);

    if (x2 < x1) {
        temp = x2;
        x2 = x1;
        x1 = temp;
    }

    if (y2 < y1) {
        temp = y2;
        y2 = y1;
        y1 = temp;
    }

    if (x1 == x2) {
        x1 -= 0.5;
        x2 += 0.5;
    }

    if (y1 == y2) {
        y1 -= 0.5;
        y2 += 0.5;
    }

    *xmin = x1;
    *xmax = x2;
    *ymin = y1;
    *ymax = y2;
}

long join(long ninp, float **x, float **y, long **l, long *selected_lines, long nsel, long target) {
    long i, source = -1;

    // Find source segment
    for(i=0;i<nsel;i++) {
        if (selected_lines[i] == SEL) {
            if (source == -1) {
                source = i;
                continue;
            }
            warn("Cannot handle more than one selected line for merge !");
            return ninp;
        }
    }

    if (source == -1) {
        warn("Need one selected segment to merge.");
        return ninp;
    }

    // Check that we have distinct segments
    if (source == target) {
        warn("Cannot merge segment with itself");
        return ninp;
    }

    // Merge
    long s1,s2;
    long e1, e2;

    findlineedges(*l, ninp, source, &s1, &e1);
    findlineedges(*l, ninp, target, &s2, &e2);

    // Allocate space
    ninp = insert(ninp, x, y, l, e1+1, (e2 - s2 + 1));
    findlineedges(*l, ninp, target, &s2, &e2);

    // Copy target
    long to = e1 + 1;
    for(i=s2;i<=e2;i++,to++) {
        (*x)[to] = (*x)[i];
        (*y)[to] = (*y)[i];
        (*l)[to] = (*l)[s1];
    }

    // Clean up
    selected_lines[target] = SEL;
    selected_lines[source] = NSEL;
    ninp = deletesegments(ninp, x, y, l, selected_lines, nsel);
    selected_lines[source] = SEL;

    return ninp;
}

long closepol(long ninp, float **x, float **y, long **l, long i) {
    long s, e;

    findlineedges(*l,ninp,i,&s,&e);
    if (s == -1 || e == -1) {
        warn("Cannot find polygons edges here.");
        return ninp;
    }

    ninp = insert(ninp, x, y, l , e + 1, 1);
    (*x)[e+1] = (*x)[s];
    (*y)[e+1] = (*y)[s];
    (*l)[e+1] = (*l)[s];

    return ninp;
}

long ctlplot(long argc, char **argv)
{
    // Data helpers
    float xmin,xmax;
    float ymin,ymax;
    float ax = 0.0, ay = 0.0;
    char ch;
    long i, j;

    float xt,yt,xb,yb;

    // Draw controls
    long drawpoints = 1;
    long drawline = 1;
    long drawse = 0;
    long drawlabel = 1;
    long drawall = 1;

    // Selection
    long *selected_lines = NULL;
    long nsel = 0;
    long saved = 0;

    // Data Holders
    float *x = NULL;
    float *y = NULL;
    long  *l = NULL;
    long ninp = 0;

    // Data Undo Holders
    float *nx = NULL;
    float *ny = NULL;
    long  *nl = NULL;
    long nninp = 0;

    // Import file
    for (i = 1; i < argc; i ++) {
        fprintf(stderr,"Loading %s\n", argv[i]);
        long ret = 0;
        ret = importfile(argv[i],&x,&y,&l, ninp);
        if (ret <= 0) continue;
        ninp = ret;
    }

    // Prepare selected lines
    selected_lines = reindex(ninp, l, &selected_lines, &nsel, 0);

    saved = 1;

    opengr();

    resetzoom(x,y,ninp,&xmin,&xmax,&ymin,&ymax);
    cpgenv(xmin,xmax,ymin,ymax,0,0);

    ch='A';
    while (ch!='Q')
    {
        cpgeras();
        cpgenv(xmin, xmax, ymin, ymax, 0, 0);
        plotme(argc, argv, ninp,x,y,l,selected_lines,drawpoints, drawline, drawse,drawlabel, saved, drawall);
        cpgband (7, 0, 0, 0, &ax, &ay, &ch);
        ch = toupper (ch);
        switch (ch)
        {
        case 'C': // Close
            i = findline(ax,ay,x,y,l,ninp);
            if (i == -1) break;
            nninp = saveundo(ninp, x, y, l, &nx, &ny, &nl);
            ninp = closepol(ninp, &x, &y, &l, i);
            selected_lines = reindex(ninp,l,&selected_lines,&nsel,1);
            break;

        case 'J': // Join
            i = findline(ax,ay,x,y,l,ninp);
            if (i == -1) break;
            nninp = saveundo(ninp, x, y, l, &nx, &ny, &nl);
            ninp = join(ninp, &x, &y, &l, selected_lines, nsel, i);
            selected_lines = reindex(ninp,l,&selected_lines,&nsel,1);
            break;

        case 'Y': // UNDO
            if (nl != NULL) {
                if (x != NULL) free(x);
                if (y != NULL) free(y);
                if (l != NULL) free(l);

                x = nx;
                y = ny;
                l = nl;
                ninp = nninp;

                nx = NULL;
                ny = NULL;
                nl  = NULL;
                nninp = 0;

                selected_lines = reindex(ninp,l, &selected_lines, &nsel,0);
            }
            break;

        case 'N': // ADD
            nninp = saveundo(ninp, x, y, l, &nx, &ny, &nl);
            ninp = addsegment(ninp,&x,&y,&l);
            selected_lines = reindex(ninp,l,&selected_lines,&nsel,1);
            saved = 0;
            break;

        case 'D': // DEL
            nninp = saveundo(ninp, x, y, l, &nx, &ny, &nl);
            ninp = deletesegments(ninp,&x,&y,&l,selected_lines, nsel);
            selected_lines = reindex(ninp,l,&selected_lines,&nsel,1);
            saved = 0;
            break;

        case 'X': // ZOOM
            zoom(x,y,ninp,ax,ay,&xmin,&xmax,&ymin,&ymax);
            break;

        case 'B': // BREAK
            nninp = saveundo(ninp, x, y, l, &nx, &ny, &nl);
            i=findline(ax,ay,x,y,l,ninp);
            if (i == -1) break;
            if (selected_lines[i]==NSEL) {
                fprintf(stderr,"ooops: This line is not selected !\n");
            } else {
                j=findindex(ax,ay,x,y,l,ninp);
                if (j == -1) break;
                {
                    long lmin,lmax;
                    dminmax(l,ninp,&lmin,&lmax);
                    lmax ++;
                    while (l[j] == i) {
                        l[j]=lmax;
                        j++;
                    }
                    selected_lines = reindex(ninp,l,&selected_lines,&nsel,1);
                }
            }
            saved = 0;
            break;

        case 'W': // Wrap
            wrap(ninp, x);
            resetzoom(x,y,ninp,&xmin,&xmax,&ymin,&ymax);
            break;

        case 'U': // Un-wrap
            unwrap(ninp, x);
            resetzoom(x,y,ninp,&xmin,&xmax,&ymin,&ymax);
            break;

        case ' ': // Select
            i=findline(ax,ay,x,y,l,ninp);
            if (i == -1) break;
            if (selected_lines[i]==NSEL)
                selected_lines[i]=SEL;
            else
                selected_lines[i]=NSEL;
            break;

        case 'R': // Revert segment direction
            nninp = saveundo(ninp, x, y, l, &nx, &ny, &nl);
            i=findline(ax,ay,x,y,l,ninp);
            if (i == -1) break;
            reverse_line(ninp,x,y,l,i);
            saved = 0;
            break;

        case 'T': // Transpose
            reversexy(ninp,x,y);
            resetzoom(x,y,ninp,&xmin,&xmax,&ymin,&ymax);
            saved = 0;
            break;

        case 'S': // Select polygon by edges
            xt=ax;
            yt=ay;
            cpgband (2, 0, xt, yt, &ax, &ay, &ch);
            xb=ax;
            yb=ay;
            getpointsinside(ninp,x,y,l,selected_lines,xt,yt,xb,yb);
            break;

        case 'I': // Invert selection globally
            for(i=0;i<nsel;i++)
                selected_lines[i] = (selected_lines[i] == SEL) ? NSEL : SEL;
            break;

        case 'Z': // Clear all selection
            for(i=0;i<nsel;i++)
                selected_lines[i]=NSEL;
            break;

        case 'Q': /* Exit !*/
            if (saved == 0) {
                cpgsvp(0.35,0.95,0.05,0.35);
                cpgswin(0.0,1.0,0.0,1.0);
                cpgrect(0.0,1.0,0.0,1.0);
                cpgsci(0);
                cpgrect(0.01,.99,0.01,0.99);
                cpgsci(1);
                if (argc <= 2) {
                    cpgmtxt("T",-1.0,0.02,.0,"File was modified, Save ?");
                    cpgband (7, 0, 0, 0, &ax, &ay, &ch);
                    ch = toupper (ch);
                    if (ch == 'Y') {
                        savefile(argv[1], ninp, x, y, l);
                        ch = 'Q';
                    } else {
                        cpgmtxt("T",-2.0,0.02,.0,"Press Q one more time to quit");
                        cpgband (7, 0, 0, 0, &ax, &ay, &ch);
                        ch = toupper (ch);
                    }
                } else {
                    cpgmtxt("T",-1.0,0.02,.0,"Loaded data was modified and more than one file is loaded.");
                    cpgmtxt("T",-2.0,0.02,.0,"Please make sure that you exported your modifications");
                    cpgmtxt("T",-3.0,0.02,.0,"before leaving.");
                    cpgmtxt("T",-5.0,0.02,.0,"Hit 'Q' again to really quit, any other key to continue ... ");

                    cpgband (7, 0, 0, 0, &ax, &ay, &ch);
                    ch = toupper (ch);
                }
            }
            break;

        case '.': // Draw all
            drawall = !drawall;
            break;

        case 'E': // Export selected
            exportline(ninp,x,y,l,selected_lines);
            break;

        case 'P':
            drawpoints = !drawpoints;
            break;

        case 'M':
            drawse= !drawse;
            break;

        case 'L':
            drawline = !drawline;
            break;;

        case '1':
            resizemax(0.85, 3.0 / 4.0);
            break;

        case '2':
            resizemax(0.55, 3.0 / 4.0);
            break;

        case '3':
            resizemax(0.35, 3.0 / 4.0);
            break;

        case '4':
            resizemax(0.35, -1);
            break;

        case '5':
            resizemax(0.55, -1);
            break;

        case '6':
            resizemax(0.85, -1);
            break;

        case '+': {
            char *newfile = getfilename();
            ninp = importfile(newfile, &x, &y, &l, ninp);
            selected_lines = reindex(ninp,l,&selected_lines,&nsel,1);
            free(newfile);
            newfile = NULL;
        }
            break;

        case 'H':
            cpgsch(0.75);
            cpgsvp(0.25,0.95,0.05,0.40);
            cpgswin(0.0,1.0,0.0,1.0);
            cpgrect(0.0,1.0,0.0,1.0);
            cpgsci(0);
            cpgrect(0.01,.99,0.01,0.99);
            cpgsci(1);

            cpgsci(5);
            float xpos = 0.02;
            cpgmtxt("T",-1.0+0.0,xpos,0.0,"General Controls:");
            cpgsci(1);
            cpgmtxt("T",-2.0-0.3,xpos,0.0,"q - Quit");
            cpgmtxt("T",-3.0-0.3,xpos,0.0,"h - Help");
            cpgmtxt("T",-4.0-0.3,xpos,0.0,"e - Export");
            cpgmtxt("T",-5.0-0.3,xpos,0.0,"y - Undo");
            cpgmtxt("T",-6.0-0.3,xpos,0.0,"+ - Import a file into memory");
            cpgmtxt("T",-6.0-0.3,xpos,0.0,"1-6 - Change plot window size");

            cpgsci(5);
            cpgmtxt("T",-8.0+0.0,xpos,0.0,"Zoom/Plot Controls:");
            cpgsci(1);
            cpgmtxt("T",-9.0-0.3,xpos,0.0,"x - Zoom (right mouse click)");
            cpgmtxt("T",-10.0-0.3,xpos,0.0,"xx - Reset Zoom");
            cpgmtxt("T",-11.0-0.3,xpos,0.0,"p - Show points On/Off");
            cpgmtxt("T",-12.0-0.3,xpos,0.0,"l - Show lines On/Off");
            cpgmtxt("T",-13.0-0.3,xpos,0.0,"m - Show SE markers On/Off");
            cpgmtxt("T",-14.0-0.3,xpos,0.0,". - Show only selected");

            cpgsci(5);
            xpos = 0.35;
            cpgmtxt("T",-1.0-0.0,xpos,0.0,"Modification Commands:");
            cpgsci(1);
            cpgmtxt("T",-2.0-0.3 ,xpos,0.0 ,"t - Transpose XY");
            cpgmtxt("T",-3.0-0.3 ,xpos,0.0,"r - Revert segment direction");
            cpgmtxt("T",-4.0-0.3 ,xpos,0.0,"b - split segment");
            cpgmtxt("T",-5.0-0.3 ,xpos,0.0,"n - new segment");
            cpgmtxt("T",-6.0-0.3 ,xpos,0.0,"d - delete segment");
            cpgmtxt("T",-7.0-0.3 ,xpos,0.0,"u - Unwrap coordinates");
            cpgmtxt("T",-8.0-0.3 ,xpos,0.0,"w - Wrap coordinates");
            cpgmtxt("T",-9.0-0.3 ,xpos,0.0,"j - Join current and selected");
            cpgmtxt("T",-10.0-0.3,xpos,0.0,"c - Close polygon");

            cpgsci(5);
            xpos = 0.67;
            cpgmtxt("T",-1.0+0.0,xpos,0.0,"Selection Controls:");
            cpgsci(1);
            cpgmtxt("T",-2.0-0.3,xpos,0.0,"SPACE - Select near segment");
            cpgmtxt("T",-3.0-0.3,xpos,0.0,"i - Invert Selection");
            cpgmtxt("T",-4.0-0.3,xpos,0.0,"s - Select by Region ");
            cpgmtxt("T",-5.0-0.3,xpos,0.0,"    (point intersect)");
            cpgmtxt("T",-6.0-0.3,xpos,0.0,"z - Zero selection");

            // Finish Help
            cpgband (7, 0, 0, 0, &ax, &ay, &ch);
            cpgsch(1.0);
            break;

        default:
            break;
        }
    }

    cpgclos();

    if (selected_lines!=NULL)
    {
        free(selected_lines);
        selected_lines=NULL;
    }

    if (x!=NULL)
    {
        free(x);
        x=NULL;
    }

    if (y!=NULL)
    {
        free(y);
        y=NULL;
    }

    if (l!=NULL)
    {
        free(l);
        l=NULL;
    }

    return 0;
}

long main(long argc, char **argv)
{

   /*
    * VERSION 0.2
    *
    * Code developed by Marcelo Bianchi <m.tchelo@gmail.com>
    */
    fprintf(stderr,"extract-map-parts version 0.2\n");
    if (argc < 2) {
        fprintf(stderr,"Need at least one XY file to load.\n");
        return 1;
    }

    long ret;
    ret = ctlplot(argc, argv);
    return ret;
}
