/*
***************************************************************************
*
* Author: Teunis van Beelen
*
* Copyright (C) 2011, 2012, 2013, 2014, 2015, 2016 Teunis van Beelen
*
* Email: teuniz@gmail.com
*
***************************************************************************
*
* This program is free software: you can redistribute it and/or modify
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
*
***************************************************************************
*/



#include "ecg_export.h"
#include "filteredblockread.h"



UI_ECGExport::UI_ECGExport(UI_Mainwindow *parent)
{
  mainwindow = parent;
/*
  myobjectDialog = new QDialog(w_parent);

  myobjectDialog->setMinimumSize(400, 445);
  myobjectDialog->setMaximumSize(400, 445);
  myobjectDialog->setWindowTitle("Export RR-intervals");
  myobjectDialog->setModal(true);
  myobjectDialog->setAttribute(Qt::WA_DeleteOnClose, true);

  list = new QListWidget(myobjectDialog);
  list->setGeometry(20, 20, 130, 350);
  list->setSelectionBehavior(QAbstractItemView::SelectRows);
  list->setSelectionMode(QAbstractItemView::SingleSelection);

  groupBox1 = new QGroupBox(myobjectDialog);
  groupBox1->setGeometry(170, 20, 210, 120);
  groupBox1->setTitle("Output format");

  radioButton1 = new QRadioButton("RR interval");
  radioButton2 = new QRadioButton("R Onset + RR interval");
  radioButton3 = new QRadioButton("R Onset");
  radioButton2->setChecked(true);

  checkBox1 = new QCheckBox("Whole recording", myobjectDialog);
  checkBox1->setGeometry(170, 160, 210, 25);
  checkBox1->setTristate(false);

  checkBox2 = new QCheckBox("Don't write to file,\n"
                            "import as annotations instead",
                            myobjectDialog);
  checkBox2->setGeometry(170, 200, 210, 40);
  checkBox2->setTristate(false);

  vbox1 = new QVBoxLayout;
  vbox1->addWidget(radioButton1);
  vbox1->addWidget(radioButton2);
  vbox1->addWidget(radioButton3);

  groupBox1->setLayout(vbox1);

  startButton = new QPushButton(myobjectDialog);
  startButton->setGeometry(20, 400, 150, 25);
  startButton->setText("Import/Export");

  cancelButton = new QPushButton(myobjectDialog);
  cancelButton->setGeometry(280, 400, 100, 25);
  cancelButton->setText("Cancel");
  */
  load_signalcomps();

  //QObject::connect(cancelButton, SIGNAL(clicked()), myobjectDialog, SLOT(close()));
 // QObject::connect(startButton,  SIGNAL(clicked()), this,           SLOT(Export_RR_intervals()));

  //myobjectDialog->exec();
}

void UI_ECGExport::output_entry(FILE * file, config_t * config, int subject, long long timestamp, double time, double value, const char *label) {
	// Output using ground file format: 
	//		subject_id (int) | timestamp (int) | time_seconds (int) | HR (double) or RR | label (string)

	fprintf(file, "%d ", config->subject_id);

	if (config->export_timestamp) {
		fprintf(file, "%ld ", timestamp);
	}

	if (config->export_time) {
		fprintf(file, "%.4f ", time);
	}

	fprintf(file, "%.4f %s\n", value, label);
}

void UI_ECGExport::Export_RR_intervals(config_t *config)
{
	int i,
		len,
		signal_nr,
		type = -1,
		beat_cnt,
		samples_cnt,
		progress_steps,
		datarecords,
		whole_recording = 0,
		import_as_annots = 0,
		filenum = 0,
		time_int;

	char path[MAX_PATH_LENGTH],
		 str[2048];

	double *beat_interval_list,
		   *buf,
		   time;

	long long *beat_onset_list,
			  datrecs,
			  smpls_left,
			  l_time = 0LL,
			  timestamp;

  struct signalcompblock *signalcomp;

  FILE *outputfile;

  signal_nr = 1;

  if((signal_nr < 0) || (signal_nr >= mainwindow->signalcomps))
  {
    printf("Invalid signalcomp number\n");
	exit(5);
  }

  signalcomp = mainwindow->signalcomp[signal_nr];

  if(signalcomp->ecg_filter == NULL)
  {
    printf("Heart Rate detection is not activated for the selected signal: %s", signalcomp->signallabel);
	exit(5);
  }

  //if(checkBox1->checkState() == Qt::Checked)
  {
    whole_recording = 1;

    class FilteredBlockReadClass blockrd;

    buf = blockrd.init_signalcomp(signalcomp, 1, 0);
    if(buf == NULL)
    {
      printf("Error, can not initialize FilteredBlockReadClass.");
	  exit(8);
    }

    samples_cnt = blockrd.samples_in_buf();
    if(samples_cnt < 1)
    {
      printf("Error, samples_cnt is < 1.");
	  exit(8);
    }

    filenum = signalcomp->filenum;

    reset_ecg_filter(signalcomp->ecg_filter);

    datarecords = signalcomp->edfhdr->datarecords;

    for(i=0; i<signalcomp->edfhdr->datarecords; i++)
    {
      if(blockrd.process_signalcomp(i) != 0)
      {
        printf("\nError while reading file.\n");
		exit(8);
      }
    }
  }

  beat_cnt = ecg_filter_get_beat_cnt(signalcomp->ecg_filter);

  beat_onset_list = ecg_filter_get_onset_beatlist(signalcomp->ecg_filter);

  beat_interval_list = ecg_filter_get_interval_beatlist(signalcomp->ecg_filter);

  //printf("beat_cnt = %d\n", beat_cnt);

  if(beat_cnt < 4)
  {
    printf("Error, not enough beats. how?!");
	exit(5);
  }

  /*
  if(import_as_annots)
  {
    for(i=0; i<beat_cnt; i++)
    {
      if(whole_recording)
      {
        l_time = 0LL;
      }
      else
      {
        l_time = signalcomp->edfhdr->viewtime;
      }

      if(l_time < 0LL)
      {
        l_time = 0LL;
      }

      datrecs = beat_onset_list[i] / signalcomp->edfhdr->edfparam[signalcomp->edfsignal[0]].smp_per_record;

      smpls_left = beat_onset_list[i] % signalcomp->edfhdr->edfparam[signalcomp->edfsignal[0]].smp_per_record;

      l_time += (datrecs * signalcomp->edfhdr->long_data_record_duration);

      l_time += ((smpls_left * signalcomp->edfhdr->long_data_record_duration) / signalcomp->edfhdr->edfparam[signalcomp->edfsignal[0]].smp_per_record);

      if(!whole_recording)
      {
        l_time += (mainwindow->edfheaderlist[mainwindow->sel_viewtime]->viewtime - signalcomp->edfhdr->viewtime);
      }

      annotation = (struct annotationblock *)calloc(1, sizeof(struct annotationblock));
      if(annotation == NULL)
      {
        printf("A memory allocation error occurred (annotation).");
        messagewindow.exec();
        return;
      }
      annotation->onset = l_time;
      strncpy(annotation->annotation, "R-onset", MAX_ANNOTATION_LEN);
      annotation->annotation[MAX_ANNOTATION_LEN] = 0;
      edfplus_annotation_add_item(&mainwindow->annotationlist[filenum], annotation);
    }

    if(mainwindow->annotations_dock[signalcomp->filenum] == NULL)
    {
      mainwindow->annotations_dock[filenum] = new UI_Annotationswindow(filenum, mainwindow);

      mainwindow->addDockWidget(Qt::RightDockWidgetArea, mainwindow->annotations_dock[filenum]->docklist, Qt::Vertical);

      if(!mainwindow->annotationlist[filenum])
      {
        mainwindow->annotations_dock[filenum]->docklist->hide();
      }
    }

    if(mainwindow->annotationlist[filenum])
    {
      mainwindow->annotations_dock[filenum]->docklist->show();

      mainwindow->annotations_edited = 1;

      mainwindow->annotations_dock[filenum]->updateList();

      mainwindow->save_act->setEnabled(true);
    }
  }
  else
  {
  */

	if (config->output_file == "") {
		// No ouput file. Print things to stdout
		outputfile = stdout;

	} else {
		strcpy(path, config->output_file.c_str());

		if (!strcmp(path, "")) {
			printf("Invalid output file: \"%s\"\n", path);
			exit(5);
		}

		outputfile = fopeno(path, "wb");

		if (outputfile == NULL) {
			printf("Unable to open output file for writing.");
			exit(5);
		}
	}

    //if(radioButton1->isChecked() == true)
   // {
    //  type = 1;
    //}

	/*
    if(radioButton2->isChecked() == true)
    {
      type = 2;
    }

    if(radioButton3->isChecked() == true)
    {
      type = 3;
    }
	*/

	//type = config->export_hr ? 4 : 2;

    //if(type == 1)
    //{
    //  for(i=0; i<beat_cnt; i++)
    //  {
    //    fprintf(outputfile, "%.4f\n", beat_interval_list[i]);
    //  }
    //}

    for(i=0; i<beat_cnt; i++) {
		l_time = whole_recording ? 0LL : signalcomp->edfhdr->viewtime;

		if(l_time < 0LL) {
			l_time = 0LL;
		}

		datrecs = beat_onset_list[i] / signalcomp->edfhdr->edfparam[signalcomp->edfsignal[0]].smp_per_record;
		smpls_left = beat_onset_list[i] % signalcomp->edfhdr->edfparam[signalcomp->edfsignal[0]].smp_per_record;

		l_time += (datrecs * signalcomp->edfhdr->long_data_record_duration);
		l_time += ((smpls_left * signalcomp->edfhdr->long_data_record_duration) / signalcomp->edfhdr->edfparam[signalcomp->edfsignal[0]].smp_per_record);

		if(!whole_recording) {
			l_time += (mainwindow->edfheaderlist[mainwindow->sel_viewtime]->viewtime - signalcomp->edfhdr->viewtime);
		}

		timestamp = mainwindow->edfheaderlist[mainwindow->sel_viewtime]->utc_starttime;
		time = ((double)l_time) / TIME_DIMENSION;
		time_int = (int)time;

		if(config->export_rr) {
			// We are exporting RR info.
			output_entry(outputfile, config, config->subject_id, timestamp + time_int, time, beat_interval_list[i], config->label.c_str());

		} else if (config->export_hr) {
			// We are exporting HR info.

			// If this is the very first HR info and the time is greater
			// then zero, it means we are missing info from 0 until the time
			// of the current HR record. Let's add add entries until we reach
			// the right time
			if (i == 0 && time_int != 0) {
				for (int t = 0; t < time_int; t++) {
					output_entry(outputfile, config, config->subject_id, timestamp + t, t, 0, config->label.c_str());
				}
			}

			// If the time we have is different from the last one
			// read, we are dealing with beats in different seconds.

			output_entry(outputfile, config, config->subject_id, timestamp + time_int, time, 60.0 / beat_interval_list[i], config->label.c_str());
		}
    }
	
	if (outputfile != stdout) {
		fclose(outputfile);
		printf("Done! Data has been extracted to: \"%s\"", path);
	}
  //}

  //myobjectDialog->close();

  // Keep the buffer in case somebody else wants to use it.
  //reset_ecg_filter(signalcomp->ecg_filter);

  mainwindow->setup_viewbuf();
}


void UI_ECGExport::load_signalcomps(void)
{
  int i;

  for(i=0; i<mainwindow->signalcomps; i++)
  {
    printf(" processing: %s\n", mainwindow->signalcomp[i]->signallabel);
  }
}






















