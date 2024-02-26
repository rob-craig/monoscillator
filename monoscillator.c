/* This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <gtk/gtk.h>

#include <jack/jack.h>
#include <jack/weakmacros.h>
#include <jack/types.h>
#include <jack/midiport.h>

#include <stdlib.h>
#include <math.h>

typedef jack_default_audio_sample_t sample_t;

/* variables */
double volume;
static const double PI = 3.14159265359;

jack_client_t *client;

jack_port_t *output_port;
jack_port_t *midi_input_port;
void* midi_port_buffer;

jack_nframes_t samplerate;
jack_nframes_t offset;     //number of samples generated so far. initially zero.
jack_midi_event_t currentMidiEvent;

int frequency; //frequency of the current note being played.
int prevFreq;
jack_midi_data_t note;
jack_midi_data_t prevNote;
jack_midi_data_t *pressedNotes;
int numPressedNotes;
GtkWidget *mainWindow, *volumeScale;

/* functions */

double midiToFreq(jack_midi_data_t note);
void removeNote(jack_midi_data_t rm_note);
void addNote(jack_midi_data_t add_note);
void adjust_volume (GtkWidget *vScale,
                    gpointer   callback_data);
int get_srate(jack_nframes_t nframes, void* arg);

double midiToFreq(jack_midi_data_t note){
    // converts MIDI note code to a frequency in Hz (cycles per second) 
    return pow(2.0,((double)note-69.0)/12.0)*440.0;
}

void addNote(jack_midi_data_t add_note){
    printf("numPressedNotes=%d\n", numPressedNotes);
    if(numPressedNotes > 128){
        fprintf(stderr, "Too many simultaneous notes pressed");
        exit(1);
    } else {
        *(pressedNotes+numPressedNotes)=add_note;
        numPressedNotes++;
    }

}

void removeNote(jack_midi_data_t rm_note){
    
    if(numPressedNotes < 0){
        fprintf(stderr, "Tried to remove a note but none were found");
        exit(1);
    } else {
        for (int i=0;i<numPressedNotes;i++){
            if (pressedNotes[i] == rm_note){
                for(int j=i;j<numPressedNotes;j++){   
                    pressedNotes[j]=pressedNotes[j+1];    
                }
                break;
            }       
        }                   
        numPressedNotes--;
    }
}

void adjust_volume (GtkWidget *vScale,
                    gpointer   callback_data )
{

    volume = (gtk_range_get_value(GTK_RANGE(vScale))/100.0);      
    printf("%f\n", volume);
}

int get_srate(jack_nframes_t nframes, void* arg){
    printf("the sample rate is now %lu Hz\n", (long)nframes);
    samplerate = nframes;
    return 0;
}

int process(jack_nframes_t nframes, void *arg){
    //get midi events
    jack_nframes_t i;
    
    midi_port_buffer = jack_port_get_buffer(midi_input_port, nframes);

    for(i=0;i<jack_midi_get_event_count(midi_port_buffer);i++){
        if (!jack_midi_event_get(&currentMidiEvent, midi_port_buffer, i)){
            if(currentMidiEvent.size == 3){
                if(currentMidiEvent.buffer[0]==0x90){ //note on
                    addNote(currentMidiEvent.buffer[1]);
                } else if (currentMidiEvent.buffer[0]==0x80){ //note off
                    removeNote(currentMidiEvent.buffer[1]);
                }
            }
        }
    }

    //set the frequency to the most recently pressed note
    frequency = 0;
    if (numPressedNotes>0){
        frequency = midiToFreq(pressedNotes[numPressedNotes-1]);
    }

    sample_t *out = (sample_t *)jack_port_get_buffer(output_port, nframes);

    for(i=0;i<nframes;i++){
        out[i]=(sin(2*PI*frequency*offset/samplerate))*volume;
        offset++;
    }
    return 0;
}

int main(int argc, char* argv[])
{
    gtk_init(&argc,&argv); // initialize gtk

    volume = 0.5;
    pressedNotes = malloc(sizeof(jack_midi_data_t)*128); 
    numPressedNotes = 0;
    offset=0;
    frequency=0; //zero frequency generates silence

    if ((client = jack_client_open("monoscillator", JackUseExactName, NULL)) == 0) {
        printf("Error registering monoscillator with JACK.\n");
        exit(1);
    }

    jack_set_process_callback(client, process, NULL);
    jack_set_sample_rate_callback(client, get_srate, NULL);
    samplerate=jack_get_sample_rate(client);
    printf("Audio sample rate: %lu Hz\n", (long)samplerate);

    // register MIDI in and audio out ports
    output_port = jack_port_register(client, "output", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
    midi_input_port = jack_port_register(client, "midi_in", JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0);
    
    if (jack_activate (client)) {
        printf("Failed to activate JACK client.\n");
        exit(1);
    }

    // set up gui
    mainWindow  = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(mainWindow), "monoscillator");
    gtk_window_set_default_size(GTK_WINDOW(mainWindow),
                                35,
                                200);
    
    volumeScale = gtk_scale_new_with_range(GTK_ORIENTATION_VERTICAL,
                                           0,
                                           100,
                                           5);
    gtk_range_set_inverted(GTK_RANGE(volumeScale), TRUE); 
      
    g_signal_connect(mainWindow,    //exit when "x" button is clicked 
                     "delete-event", 
                     G_CALLBACK(gtk_main_quit), 
                     NULL);    
    g_signal_connect(volumeScale,  //call the volume_changed function when volumeScale moves 
                     "value-changed",
                     G_CALLBACK(adjust_volume),
                     NULL);

    gtk_container_add(GTK_CONTAINER(mainWindow), volumeScale);
    gtk_widget_show_all(mainWindow); 

    gtk_main(); //call the gtk main loop 
    jack_client_close(client);
    free(pressedNotes);
    return 0;    
}

