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

#define USEL 0
#define NSEL 1
#define  SEL 2

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

long exportline(long ninp,float *x, float *y, long *l, long *selected_lines)
{
    long i,pl,nexp=0;
    FILE *out;
    char *filename=NULL;

    if (ninp == 0) return;

    filename=getfilename();
    out=fopen(filename,"w");
    if (out == NULL )
    {
        fprintf(stderr,"Export Failed !\n");
        return -1;
    }

    i=0;
    pl=0;
    if (selected_lines[l[i]]==SEL)
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

    fprintf(stderr,"Exported %d lines to file %s\n",nexp,filename);
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

    if (ninp == 0) return 0;

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

    if (ninp == 0) return 0;

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

long plotme(long argc, char ** argv, long ninp,float *x,float *y, long *l,long *selected_lines, long drawpoints, long drawline, long drawse, long drawlabel, long saved)
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

    if (ninp == 0) return;
    if (x == NULL) return;
    if (y == NULL) return;
    if (l == NULL) return;
    if (selected_lines == NULL) return;

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

void reindex(long ninp, long *l, long *selected_lines, long nsel, long keep) {
    long seq = 0;
    long current = -1;
    long i;

    long *stemp = malloc(sizeof(long) * nsel);
    for(i = 0; i < nsel; i++)
        stemp[i] = NSEL;

    if (ninp > 0) {
        current = l[0];
        if (keep) stemp[seq] = selected_lines[current];
        for(i=0;i<ninp;i++) {
            if (l[i] != current) {
                seq ++;
                current = l[i];
                if (keep) stemp[seq] = selected_lines[current];
            }
            l[i] = seq;
        }
    }

    for(i = 0; i < nsel; i++)
        selected_lines[i] = stemp[i];

    free(stemp);
    stemp = NULL;
    return;
}

long cut(long ninp, float **x, float **y, long **l, long n1, long n2) {
    long amount = (ninp - n2 - 1);
    long left = ninp - (n2 - n1 + 1);

    float *xm = *x;
    float *ym = *y;
    long *lm = *l;

    memmove(&xm[n1],&xm[n2+1],sizeof(float) * amount);
    memmove(&ym[n1],&ym[n2+1],sizeof(float) * amount);
    memmove(&lm[n1],&lm[n2+1],sizeof(long) * amount);

    *x = (float*) realloc(*x, sizeof(float) * left);
    *y = (float*) realloc(*y, sizeof(float) * left);
    *l = (long*) realloc(*l, sizeof(long) * left);

    return left;
}

long deletesegments(long ninp, float **x, float **y, long **l, long *selected_lines, long nsel) {
    long i = 0;

    while (i < nsel) {
        if (selected_lines[i] == SEL) {
            long n1 = -1;
            long n2 = -1;
            long j;
            for(j=0;j<ninp && n2 == -1;j++) {
                if ((*l)[j] == i && n1 == -1) n1 = j;
                if (n1 != -1 && (*l)[j] != i) n2 = j - 1;
            }
            if (n1 == -1) {
                fprintf(stderr,"Cannot find segments edjes.");
                return;
            }
            if (n2 == -1) n2 = ninp - 1;
            ninp = cut(ninp,x,y,l,n1,n2);
            selected_lines[i] = NSEL;
            reindex(ninp,*l, selected_lines, nsel,1);
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

long importfile(char *filename,float **x,float **y,long **lin, long ninp)
{
    FILE *in;
    long line = 0;
    long i = 0;
    long fileline = 0;
    char readline[500] = "";

    if ((in=fopen(filename,"r"))==NULL) {
        fprintf(stderr,"Cannot open and read the give file %s\n",filename);
        return -1;
    }

    // Restore the last state

    // Find the last line indexed
    dminmax(*lin,ninp,&i,&line);

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
                *lin = (long *) realloc(*lin, sizeof(long)*(i+1));
                (*x)[i] = xt;
                (*y)[i] = yt;
                (*lin)[i] = line;
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
    float x1,x2;
    float y1,y2;

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

long ctlplot(long argc, char **argv)
{
    // Data helpers
    float xmin,xmax;
    float ymin,ymax;
    float ax,ay;
    char ch;
    long i, j;

    float xt,yt,xb,yb;

    // Draw controls
    long drawpoints = 1;
    long drawline = 1;
    long drawse = 0;
    long drawlabel = 1;

    // Selection
    long *selected_lines=NULL;
    long nsel = 0;
    long saved = 0;

    // Data Holders
    float *x = NULL;
    float *y = NULL;
    long *l = NULL;
    long ninp = 0;

    // Import file
    for (i = 1; i < argc; i ++) {
        fprintf(stderr,"Loading %s\n", argv[i]);
        long ret = 0;
        ret = importfile(argv[i],&x,&y,&l, ninp);
        if (ret <= 0) continue;
        ninp = ret;
    }

    saved = 1;

    cpgopen("/xwindow");
    cpgask(0);

    resetzoom(x,y,ninp,&xmin,&xmax,&ymin,&ymax);
    cpgenv(xmin,xmax,ymin,ymax,0,0);

    // Prepare selected lines
    {
        long lmin,lmax;
        dminmax(l,ninp,&lmin,&lmax);
        nsel = (5*lmax + 1);
        selected_lines=malloc(sizeof(long)*nsel);
        if (selected_lines == NULL) return -1;
    }
    reindex(ninp, l, selected_lines, nsel, 0);

    ch='A';
    while (ch!='Q')
    {
        cpgeras();
        cpgenv(xmin,xmax,ymin,ymax,0,0);
        plotme(argc, argv, ninp,x,y,l,selected_lines,drawpoints, drawline, drawse,drawlabel, saved);
        cpgband (7, 0, 0, 0, &ax, &ay, &ch);
        ch = toupper (ch);
        switch (ch)
        {
        case 'N':
            ninp = addsegment(ninp,&x,&y,&l);
            reindex(ninp,l,selected_lines,nsel,0);
            saved = 0;
            break;

        case 'D':
            ninp = deletesegments(ninp,&x,&y,&l,selected_lines, nsel);
            saved = 0;
            break;

        case 'X':
            zoom(x,y,ninp,ax,ay,&xmin,&xmax,&ymin,&ymax);
            break;

        case 'B':
            // Split a line segment
            i=findline(ax,ay,x,y,l,ninp);
            if (selected_lines[i]==NSEL) {
                fprintf(stderr,"ooops: This line is not selected !\n");
            } else {
                j=findindex(ax,ay,x,y,l,ninp);
                {
                    long lmin,lmax;
                    dminmax(l,ninp,&lmin,&lmax);
                    lmax ++;
                    while (l[j] == i) {
                        l[j]=lmax;
                        j++;
                    }
                    reindex(ninp, l, selected_lines, nsel,1);
                }
            }
            saved = 0;
            break;

        case 'W':
            wrap(ninp, x);
            resetzoom(x,y,ninp,&xmin,&xmax,&ymin,&ymax);
            break;

        case 'U':
            unwrap(ninp, x);
            resetzoom(x,y,ninp,&xmin,&xmax,&ymin,&ymax);
            break;

        case ' ':
            // Invert selection for close line
            i=findline(ax,ay,x,y,l,ninp);
            if (selected_lines[i]==NSEL)
                selected_lines[i]=SEL;
            else
                selected_lines[i]=NSEL;
            break;

        case 'R':
            // Reverse Line
            i=findline(ax,ay,x,y,l,ninp);
            reverse_line(ninp,x,y,l,i);
            saved = 0;
            break;

        case 'T':
            // Change XZ coordinates
            reversexy(ninp,x,y);
            resetzoom(x,y,ninp,&xmin,&xmax,&ymin,&ymax);
            saved = 0;
            break;

        case 'S':
            // Select polygon by edges
            xt=ax;
            yt=ay;
            cpgband (2, 0, xt, yt, &ax, &ay, &ch);
            xb=ax;
            yb=ay;
            getpointsinside(ninp,x,y,l,selected_lines,xt,yt,xb,yb);
            break;

        case 'I':
            // Invert selection globally
            for(i=0;i<nsel;i++)
                selected_lines[i] = (selected_lines[i] == SEL) ? NSEL : SEL;
            break;

        case 'Z':
            // Clear all selection
            for(i=0;i<nsel;i++)
                selected_lines[i]=NSEL;
            break;

        case 'Q':
            /* Exit !*/
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

        case 'E':
            // Export selected
            exportline(ninp,x,y,l,selected_lines);
            break;

        case 'P':
            // preview the export
            drawpoints = !drawpoints;
            break;

        case 'M':
            // preview the export
            drawse= !drawse;
            break;

        case 'L':
            // Draw the lines below points or not
            drawline = !drawline;
            break;;

        case 'H':
            cpgsch(0.75);
            cpgsvp(0.45,0.95,0.05,0.40);
            cpgswin(0.0,1.0,0.0,1.0);
            cpgrect(0.0,1.0,0.0,1.0);
            cpgsci(0);
            cpgrect(0.01,.99,0.01,0.99);
            cpgsci(1);

            cpgsci(5);
            cpgmtxt("T",-1.0+0.0,0.02,0.0,"General Controls:");
            cpgsci(1);
            cpgmtxt("T",-2.0-0.3,0.02,0.0,"q - Quit");
            cpgmtxt("T",-3.0-0.3,0.02,0.0,"h - Help");
            cpgmtxt("T",-4.0-0.3,0.02,0.0,"e - Export");

            cpgsci(5);
            cpgmtxt("T",-6.0+0.0,0.02,0.0,"Zoom/Plot Controls:");
            cpgsci(1);
            cpgmtxt("T",-7.0-0.3,0.02,0.0,"x - Zoom (right mouse click)");
            cpgmtxt("T",-8.0-0.3,0.02,0.0,"xx - Reset Zoom");
            cpgmtxt("T",-9.0-0.3,0.02,0.0,"p - Show points On/Off");
            cpgmtxt("T",-10.0-0.3,0.02,0.0,"l - Show lines On/Off");
            cpgmtxt("T",-11.0-0.3,0.02,0.0,"m - Show SE markers On/Off");

            cpgsci(5);
            cpgmtxt("T",-1.0-0.0,0.52,0.0,"Modification Commands:");
            cpgsci(1);
            cpgmtxt("T",-2.0-0.3,0.52,0.0 ,"t - Transpose XY");
            cpgmtxt("T",-3.0-0.3,0.52,0.0,"r - Revert segment direction");
            cpgmtxt("T",-4.0-0.3,0.52,0.0,"b - split segment");
            cpgmtxt("T",-5.0-0.3,0.52,0.0,"n - new segment");
            cpgmtxt("T",-6.0-0.3,0.52,0.0,"d - delete segment");
            cpgmtxt("T",-7.0-0.3,0.52,0.0,"u - Unwrap coordinates");
            cpgmtxt("T",-8.0-0.3,0.52,0.0,"w - Wrap coordinates");

            cpgsci(5);
            cpgmtxt("T",-10.0+0.0,0.52,0.0,"Selection Controls:");
            cpgsci(1);
            cpgmtxt("T",-11.0-0.3,0.52,0.0,"SPACE - Select near segment");
            cpgmtxt("T",-12.0-0.3,0.52,0.0,"i - Invert Selection");
            cpgmtxt("T",-13.0-0.3,0.52,0.0,"s - Select by Region ");
            cpgmtxt("T",-14.0-0.3,0.52,0.0,"    (point intersect)");
            cpgmtxt("T",-15.0-0.3,0.52,0.0,"z - Zero selection");

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
    * VERSION 0.1
    */
    fprintf(stderr,"extract-map-parts version 0.1\n");
   
    long ret;
    ret = ctlplot(argc, argv);
    return ret;
}
