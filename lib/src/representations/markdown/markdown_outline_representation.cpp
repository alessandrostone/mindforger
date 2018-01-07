/*
 markdown_outline_representation.cpp     MindForger thinking notebook

 Copyright (C) 2016-2018 Martin Dvorak <martin.dvorak@mindforger.com>

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
#include "markdown_outline_representation.h"

#include "../../mind/ontology/ontology.h"

using namespace std;

namespace m8r {

MarkdownOutlineRepresentation::MarkdownOutlineRepresentation(Ontology& ontology)
    : ontology(ontology)
{
}

MarkdownOutlineRepresentation::~MarkdownOutlineRepresentation()
{
}

Note* MarkdownOutlineRepresentation::note(vector<MarkdownAstNodeSection*>* ast, const size_t astindex, Outline* outline)
{
    Note* note = nullptr;
    const NoteType* noteType;
    vector<string*>* body;
    const string* s;
    for(size_t i = astindex; i < ast->size(); i++) {
        s = ast->at(i)->getMetadata().getType();
        if (s) {
            // IMPROVE consider string normalization to make parsing more robust
            //std::transform(s.begin(), s.end(), s.begin(), ::tolower);
            //s[0] = toupper(s[0])
            if ((noteType = ontology.getNoteTypes().get(*s)) == nullptr) {
                noteType = ontology.getDefaultNoteType();
            }
        } else {
            noteType = ontology.getDefaultNoteType();
        }
        note = new Note{noteType, outline};
        // TODO pull pointer > do NOT copy
        if (ast->at(i)->getText() != nullptr) {
            note->setTitle(*(ast->at(i)->getText()));
        }
        note->setDepth(ast->at(i)->getDepth());
        body = ast->at(i)->moveBody();
        if (body != nullptr) {
            for (string*& bodyItem : *body) {
                note->addDescriptionLine(bodyItem);
            }
        }
        delete body;
        note->setCreated(ast->at(i)->getMetadata().getCreated());
        note->setModified(ast->at(i)->getMetadata().getModified());
        note->setRevision(ast->at(i)->getMetadata().getRevision());
        note->setRead(ast->at(i)->getMetadata().getRead());
        note->setReads(ast->at(i)->getMetadata().getReads());
        note->setProgress(ast->at(i)->getMetadata().getProgress());
        if (ast->at(i)->getMetadata().getTags().size()) {
            const Tag* t;
            for(string* s : ast->at(i)->getMetadata().getTags()) {
                t = ontology.findOrCreateTag(*s);
                note->addTag(t);
            }
        }
        if(outline) {
            outline->addNote(note);
        }
    }
    return note;
}

Outline* MarkdownOutlineRepresentation::outline(const File& file)
{
    Markdown md{&file.name};
    md.from();
    vector<MarkdownAstNodeSection*>* ast = md.moveAst();

    Outline* o = outline(ast);
    if(o) {
        o->setKey(*md.getFilePath());
        o->setBytesize(md.getFileSize());
        o->completeProperties(md.getModified());
        o->setModifiedPretty(datetimeToPrettyHtml(o->getModified()));
    }
    return o;
}

Outline* MarkdownOutlineRepresentation::outline(vector<MarkdownAstNodeSection*>* ast)
{
    Outline *outline = new Outline{ontology.getDefaultOutlineType()};
    if(ast) {
        if(ast->size()) {
            MarkdownAstNodeSection* astNode = ast->at(0);
            if(astNode->getText()!=nullptr) {
                // IMPROVE pull pointer > do NOT copy
                outline->setTitle(*(astNode->getText()));
            }
            const string* s = astNode->getMetadata().getType();
            if(s) {
                const OutlineType* outlineType;
                // IMPROVE consider string normalization to make parsing more robust
                //std::transform(s.begin(), s.end(), s.begin(), ::tolower);
                //s[0] = toupper(s[0])
                if((outlineType=ontology.getOutlineTypes().get(*s))==nullptr) {
                    outlineType = ontology.getDefaultOutlineType();
                }
                outline->setType(outlineType);
            }
            outline->setCreated(astNode->getMetadata().getCreated());
            outline->setModified(astNode->getMetadata().getModified());
            outline->setRevision(astNode->getMetadata().getRevision());
            outline->setRead(astNode->getMetadata().getRead());
            outline->setReads(astNode->getMetadata().getReads());
            outline->setImportance(astNode->getMetadata().getImportance());
            outline->setUrgency(astNode->getMetadata().getUrgency());
            outline->setProgress(astNode->getMetadata().getProgress());

            if(astNode->getMetadata().getTags().size()) {
                const Tag* t;
                for(string* s:astNode->getMetadata().getTags()) {
                    t = ontology.findOrCreateTag(*s);
                    outline->addTag(t);
                }
            }

            vector<string*>* body = ast->at(0)->moveBody();
            if(body!=nullptr) {
                // IMPROVE use body as is
                for(string*& bodyItem:*body) {
                    outline->addDescriptionLine(bodyItem);
                }
                delete body;
            }
        }

        // notes
        if(ast->size()>1) {
            note(ast, 1, outline);
        }

        // delete AST
        for(MarkdownAstNodeSection* node:*ast) {
            if(node!=nullptr) {
                delete node;
            }
        }
        delete ast;
    }

    return outline;
}

Outline* MarkdownOutlineRepresentation::header(const std::string *mdString)
{
    Markdown md{nullptr};
    md.from(mdString);
    vector<MarkdownAstNodeSection*>* ast = md.moveAst();

    return outline(ast);
}

Note* MarkdownOutlineRepresentation::note(const string *text)
{
    Markdown md{nullptr};
    md.from(text);
    vector<MarkdownAstNodeSection*>* ast = md.moveAst();

    Note* result{};
    if(ast) {
        if(ast->size()) {
            result = note(ast);
        }

        for(MarkdownAstNodeSection*& node:*ast) {
            delete node;
        }
        delete ast;
    }
    return result;
}

Note* MarkdownOutlineRepresentation::note(const File& file)
{
    string* md = fileToString(file.name);
    Note* n = note(md);
    delete md;
    return n;
}

string* MarkdownOutlineRepresentation::toHeader(const Outline* outline)
{
    string* md = new string{};
    if(outline) {
        toHeader(outline, md);
    }
    return md;
}

void MarkdownOutlineRepresentation::toHeader(const Outline* outline, string* md)
{
    if(outline) {
        if(outline->getNotes().size()) md->reserve(outline->getNotes().size()*AVG_NOTE_SIZE);

        md->append("# ");
        if(outline->getTitle().size()) {
            md->append(outline->getTitle());
        } else {
            md->append(outline->getKey());
        }

        // IMPROVE c++11: std::to_string(int)
        char buffer[50];
        md->append(" <!-- Metadata:");
        if(outline->getTags().size()) { md->append(" tags: "); md->append(to(md, outline->getTags())); md->append(";"); }
        md->append(" type: "); md->append(outline->getType()->getName()); md->append(";");
        md->append(" created: "); md->append(datetimeToString(outline->getCreated())); md->append(";");
        sprintf(buffer," reads: %d;",outline->getReads()); md->append(buffer);
        md->append(" read: "); md->append(datetimeToString(outline->getRead())); md->append(";");
        sprintf(buffer," revision: %d;",outline->getRevision()); md->append(buffer);
        md->append(" modified: "); md->append(datetimeToString(outline->getModified())); md->append(";");
        sprintf(buffer," importance: %d/5;",outline->getImportance()); md->append(buffer);
        sprintf(buffer," urgency: %d/5;",outline->getUrgency()); md->append(buffer);
        sprintf(buffer," progress: %d%%; -->\n",outline->getProgress()); md->append(buffer);

        const vector<string*>& description=outline->getDescription();
        if(description.size()) {
            for(string *s:description) {
                md->append(*s);
                md->append("\n");
            }
        }
    }
}

string* MarkdownOutlineRepresentation::to(const Outline* outline)
{
    string* md = new string{};
    md->reserve(AVG_OUTLINE_SIZE);
    return to(outline, md);
}

string* MarkdownOutlineRepresentation::to(const Outline* outline, string* md)
{
    toHeader(outline, md);
    if(outline) {
        const vector<Note*>& notes=outline->getNotes();
        if(notes.size()) {
            string noteMd{};
            for(Note *note:notes) {
                to(note, &noteMd);
                md->append(noteMd);
                noteMd.clear();
            }
        }
    }
    return md;

}

string MarkdownOutlineRepresentation::to(string* md, const vector<const Tag*>& tags)
{
    // IMPROVE suspicious - why is not md used
    UNUSED_ARG(md);

    string s;
    if(tags.size()) {
        for(const Tag* t:tags) {
            s += t->getName();
            s += ",";
        }
        s.resize(s.size()-1);
    }
    return s;
}

string* MarkdownOutlineRepresentation::to(const Note* note)
{
    string* md = new string{};
    md->reserve(AVG_NOTE_SIZE);
    return to(note, md);
}

string* MarkdownOutlineRepresentation::to(const Note* note, string* md)
{
    char buffer[50];
    for(int i=0; i<=note->getDepth(); i++) {
        md->append("#");
    }
    md->append(" ");
    if(note->getTitle().size()) {
        md->append(note->getTitle());
    } else {
        md->append("?");
    }

    md->append(" <!-- Metadata:");
    if(note->getTags().size()) { md->append(" tags: "); md->append(to(md, note->getTags())); md->append(";"); }
    md->append(" type: "); md->append(note->getType()->getName()); md->append(";");
    md->append(" created: "); md->append(datetimeToString(note->getCreated())); md->append(";");
    sprintf(buffer," reads: %d;",note->getReads()); md->append(buffer);
    md->append(" read: "); md->append(datetimeToString(note->getRead())); md->append(";");
    sprintf(buffer," revision: %d;",note->getRevision()); md->append(buffer);
    md->append(" modified: "); md->append(datetimeToString(note->getModified())); md->append(";");
    sprintf(buffer," progress: %d%%; -->\n",note->getProgress()); md->append(buffer);

    const vector<string*>& description=note->getDescription();
    if(description.size()) {
        for(string *s:description) {
            md->append(*s);
            md->append("\n");
        }
    }

    return md;
}

} /* namespace m8r */
