using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using CodEx;
using System.IO;
namespace ConsoleApplication1
{
    class Program
    {
        static void Main(string[] args)
        {
            if (args.Length != 1)
            {
              Console.WriteLine("Usage: htmlFormating.exe PATH\npath to the folder containing html books downloaded from project Gutenberg\nResult will be in the directory \"result\"\nThe source files should be in UTF-8");
            }
            else
            {
              //string directory = Console.ReadLine();
              string directory = args[0];
              string[] filePaths = Directory.GetFiles(directory,"*.htm*");
              foreach (string filepath in filePaths)
              {
                  new GutenbergExtractor(directory, filepath);
              }
            }
        }
    }
    class GutenbergExtractor
    {
        
        String file;
        String directory;
        String outputfile = "output.html"; 
        Reader reader;
        Char last = ' ';
        List<string> open;
        string title ="";
        TextWriter writer;
        string author = "";
        string produced = "from project Gutenberg";
        public GutenbergExtractor(string dir,string filepath)
        {
            Console.WriteLine(filepath);
            this.file=filepath;
            this.directory=dir;
            if (directory.Last()+"" != "\\" && directory.Last()+"" != "/"){
                directory+="\\";
            }
            //FORMATING
            reader = new Reader(file);
            string dummy;
            Find("Title: ",out dummy);
            if (!reader.EOF()) {
                title = reader.Line().Split('<','\n','\r')[0];
            }
            else{
                Console.WriteLine("error: No title "+file);
                Console.Write("Title: ");
                title = Console.ReadLine();
            }
            outputfile = Compact(title) + ".html";
            Find("Author: ", out dummy);
            if (!reader.EOF()) {
                author = reader.Line().Split('<', '\n','\r')[0]; //remove closing tag
            }
            else{
                Console.WriteLine("error: No author "+file);
                reader = new Reader(file);
            }
            if (Find("Produced by ", out dummy))
            {
                produced = "Produced by " + reader.Line().Split('<', '\n', '\r')[0] + " " + reader.Line().Split('<', '\n', '\r')[0];
            }
            reader.Dispose();
            reader = new Reader(file);
            Find("</h", out dummy);
            Find("</h", out dummy);
            reader.Line();
            string text = "<!DOCTYPE html><html><head></head><body><br><br><br><br><br><br><h1 align=\"center\"><b>" + title.ToUpper() + "</b></h1><br><h2  align=\"center\">" + author + "</h2><br><br><p align = \"center\">" + produced + "</p><hr>";
            while (!reader.EOF())
            {
                text += reader.Line() + "\n";
            }
            reader.Dispose();

            System.IO.Directory.CreateDirectory(directory + "processed\\");
            TextWriter writer = new StreamWriter(directory+"processed\\"+Compact(title)+".html", false, Encoding.UTF8);
            writer.Write(text);
            writer.Flush();
            writer.Dispose();
            findAndReplace("</p>", "</p><br>", directory + "processed\\" + Compact(title) + ".html");
            findAndReplace("&ldquo;", "“",     directory + "processed\\" + Compact(title) + ".html");
            findAndReplace("&rdquo;", "”",     directory + "processed\\" + Compact(title) + ".html");
            findAndReplace("&rsquo;", "'", directory + "processed\\" + Compact(title) + ".html");
            findAndReplace("&mdash;", "-", directory + "processed\\" + Compact(title) + ".html");

            writer.Dispose();
            reader.Dispose();

            removeLinks(directory + "processed\\" + Compact(title) + ".html");
            removeImages(directory + "processed\\" + Compact(title) + ".html");

            findAndReplace("    ", " ", file);
            while (findAndReplace("  ", " ", file)) ;

            //CONFIG
                text = "";
                text += "  class  ItemBook" + Compact(title) + ": ItemBook\r\n";
                text += "    {\r\n";
                text += "    scope = public;\r\n";
                text += "    title = \"" + title + "\";\r\n";
                text += "    author = \"" + author + "\";\r\n";
                text += "    file = \"\\DZ\\books\\Automatic\\" + outputfile + "\";\r\n";
                text += "    displayName = \"" + title + "\";\r\n";
                text += "    descriptionShort = \"" + title + "\";\r\n";
                text += "  };\r\n\r\n";

                reader.Dispose();
                writer = new StreamWriter(directory+"processed\\"+"config.txt", true, Encoding.UTF8);
                writer.Write(text);
                writer.Flush();
                writer.Dispose();
            
 
            //PROXY DEFINES
                writer = new StreamWriter(directory+"processed\\"+"proxyDefines.hpp", true, Encoding.UTF8);
                writer.Write(",\\\r\n\t\"ItemBook"+Compact(title)+"\"");
                writer.Flush();
                writer.Dispose();
            //SPAWN DEFINES
                writer = new StreamWriter(directory+"processed\\"+"spawnDefines.hpp", true, Encoding.UTF8);
                writer.Write(",\\\r\n\t\"{ItemBook"+Compact(title)+"\",PROBABILITY}");
                writer.Flush();
                writer.Dispose();

        }

        public bool findAndReplace(string find, string replace,string file) //changes the the file set in the variable file
        {
            bool result = false;
            reader = new Reader(file);
            string processedDocument = "";
            string skipped = "";
            while (!reader.EOF())
            {
                if (Find(find, out skipped))
                {
                    result = true;
                    processedDocument += skipped;
                    processedDocument += replace;
                }
                else
                {
                    processedDocument += skipped;
                }
                
            }
            reader.Dispose();
            writer = new StreamWriter(file, false, Encoding.UTF8);
            writer.Write(processedDocument);
            writer.Dispose();
            return result;
        }
        public bool Find(string what, out string skipped) //finds what, returns true if succesful. skipped contains all the read letters excluding what
        {
            List<string> suspects= new List<string>();
            LinkedList<char> skippedList= new LinkedList<char>();
            Char last;
            if (reader.EOF())
            {
                skipped = new string(skippedList.ToArray());
                return false;
            }
            while (!reader.EOF())
            {
                last = reader.Char();
                skippedList.AddLast(last);
                if (last == what[0])
                {
                    suspects.Add("");  //imagine searching the word minimize in miniminimize
                }
                for (int i = 0; i < suspects.Count;i++)
                {
                    suspects[i] += last;
                }
                for (int i = 0; i < suspects.Count;i++)
                {
                    while (last != what.ElementAt(suspects[i].Length-1))
                    {
                        suspects.RemoveAt(i);
                        if (i >= suspects.Count) break;
                    }
                }
                if (suspects.Count > 0)
                {
                    if (suspects[0].Length == what.Length)
                    {
                        skipped = "";
                        skipped = new string(skippedList.ToArray());
                        skipped=skipped.Remove(skipped.Length - what.Length);
                        return true;
                    }
                } 
            }
            skipped = new string(skippedList.ToArray());     
            return false;            
        }
        public void removeLinks(string file)
        {
            reader = new Reader(file);
            string processedDocument = "";
            string skipped;
            string dummy;
            while (Find("<a", out skipped))
            {
                processedDocument += skipped;
                Find(">", out dummy);
                Find("</a>", out skipped);
                processedDocument += skipped;
            }
            processedDocument += skipped;
            reader.Dispose();
            writer = new StreamWriter(file, false, Encoding.UTF8);
            writer.Write(processedDocument);
            writer.Flush();
            writer.Dispose();
        }
        public void removeImages(string file)
        {
            reader = new Reader(file);
            string processedDocument = "";
            string skipped;
            string dummy;
            while (Find("<img", out skipped))
            {
                processedDocument += skipped;
                Find(">", out dummy);

            }
            processedDocument += skipped;
            reader.Dispose();
            writer = new StreamWriter(file, false, Encoding.UTF8);
            writer.Write(processedDocument);
            writer.Flush();
            writer.Dispose();
        }
        public static string Compact(string what)
        {
            string[] words = what.Split();
            string result = "";
            foreach (string word in words)
            {
                result += Capitalize(word);     
            }
            List<char> resultList= result.ToList();
            int i = 0;
            bool removed;
            while (i < resultList.Count())
            {
                removed = true;
                char[] allowed = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789".ToArray();
                foreach (char a in allowed)
                {
                    if (a == resultList[i])
                    {
                        i++;
                        removed = false;
                        break;
                    } 
                }
                if (removed == true)
                {
                    resultList.RemoveAt(i);
                }

            }
            result = new string(resultList.ToArray());
            return result;
        }
        public static string Capitalize(string what)
        {
            char[] whatArray = what.ToArray();
            if (whatArray.Count() > 0)
            {
                (whatArray[0] + "").ToUpper();
                whatArray[0] = (whatArray[0] + "").ToUpper().ToArray()[0];
            }
            string result = "";
            foreach (char c in whatArray){
                result+=c;
            }
            return result;
        }
    }
}