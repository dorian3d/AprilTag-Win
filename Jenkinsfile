pipeline {
    agent { label 'linux' }
    environment {
        CCACHE_BASEDIR = "${env.WORKSPACE}"
    }
    stages {
        stage('Build') {
            parallel {
                stage('Linux') {
                    steps {
                        slackSend color: "#439FE0", message: slack_build_message("started")
                        sh "cmake -Bbuild -Hcorvis -DCMAKE_BUILD_TYPE=RelWithDebInfo -DRC_BUILD=${env.GIT_COMMIT}"
                        sh "cmake --build build -- -j"
                    }
                }
                stage('Windows 32') {
                    agent { label 'windows' }
                    steps {
                        bat "cmake -Bbuild-x32 -Hcorvis -DCMAKE_BUILD_TYPE=RelWithDebInfo -DRC_BUILD=${env.GIT_COMMIT} -A Win32"
                        bat "cmake --build build-x32 --config RelWithDebInfo"
                    }
                }
                stage('Windows 64') {
                    agent { label 'windows' }
                    steps {
                        bat "cmake -Bbuild-x64 -Hcorvis -DCMAKE_BUILD_TYPE=RelWithDebInfo -DRC_BUILD=${env.GIT_COMMIT} -A x64"
                        bat "cmake --build build-x64 --config RelWithDebInfo"
                    }
                }
            }
        }
        stage('Test') {
            steps {
                sh 'cmake --build build --target test'
            }
        }
        stage('Build slam_client') {
            steps {
                sh '''#!/bin/bash
                    source  corvis/src/movidius/mvenv
                    export MV_TOOLS_DIR=$JENKINS_HOME/mdk/tools
                    make -C corvis/src/movidius/device -j
                '''
            }
        }
        stage('Prepare Benchmark') {
            steps {
                sh 'rsync -a --link-dest=$JENKINS_HOME/benchmark_data/ $JENKINS_HOME/benchmark_data/ $(realpath .)/benchmark_data/ --exclude "*.json"'
                sh 'rsync -a --chmod=ug+w                              $JENKINS_HOME/benchmark_data/ $(realpath .)/benchmark_data/ --include "*.json" --include "*/"'
            }
        }
        stage('Check options') {
            steps {
                sh 'build/measure --benchmark benchmark_data/minimal_test_suite/'
                sh 'build/measure --disable-map --benchmark benchmark_data/minimal_test_suite/'
                sh 'build/measure --no-fast-path --benchmark benchmark_data/minimal_test_suite/'
                sh 'build/measure --async --benchmark benchmark_data/minimal_test_suite/'
                sh 'build/measure --no-gui --relocalize --save-map test.map benchmark_data/new_test_suite/WW50/VR_with_ctrl/Building/VR_RM_with_ctrl_yossi_3.stereo.rc'
                sh 'build/measure --relocalize --load-map test.map --benchmark benchmark_data/minimal_test_suite/'
            }
        }
        stage('Run benchmark') {
            steps {
                sh 'build/measure --qvga --relocalize --benchmark benchmark_data/new_test_suite/  --benchmark-output benchmark-details-$BRANCH_NAME-$GIT_COMMIT.txt'
                sh 'sed -ne /^Length/,//p benchmark-details-$BRANCH_NAME-$GIT_COMMIT.txt                           > benchmark-summary-$BRANCH_NAME-$GIT_COMMIT.txt'
                copyArtifacts projectName: "SlamTracker/master", filter: "benchmark-summary-master-*", target: "base"
                sh 'diff -u base/benchmark-summary-master-* benchmark-summary-$BRANCH_NAME-* | sed  s@base/@@g | tee benchmark-changes-$BRANCH_NAME-$GIT_COMMIT.txt'
                archiveArtifacts artifacts: "benchmark-*-$BRANCH_NAME-${GIT_COMMIT}.txt"
                withCredentials([string(credentialsId: 'slackBenchmarkToken', variable: 'SLACK_BENCHMARK_TOKEN')]) {
                    sh 'curl -s -F file=@benchmark-changes-$BRANCH_NAME-$GIT_COMMIT.txt -F channels=#slam_build -F token=$SLACK_BENCHMARK_TOKEN https://slack.com/api/files.upload'
                }
            }
        }
    }
    post {
        always {
            deleteDir()
        }
        success {
            slackSend color: "good", message: slack_build_message("succeeded (<${env.BUILD_URL}/artifact/benchmark-details-$BRANCH_NAME-${env.GIT_COMMIT}.txt|benchmark>"
                                                                  +         " <${env.BUILD_URL}/artifact/benchmark-summary-$BRANCH_NAME-${env.GIT_COMMIT}.txt|summary>)")
        }
        failure {
            slackSend color: "#FF0000", message: slack_build_message("<$BUILD_URL/console|failed>")
        }
    }
}

def slack_build_message(status) {
    def q = { s -> s.replaceAll('&','&amp;').replaceAll('<','&lt;').replaceAll('>','&gt;') }
    "<$JOB_URL|Build> <$BUILD_URL|#$BUILD_ID> of " + (env.CHANGE_ID
        ? "<${env.CHANGE_URL}|#${q env.CHANGE_ID} ${q env.CHANGE_TITLE}> for `${q env.CHANGE_TARGET}` by `${env.CHANGE_AUTHOR}`"
        : "<${env.GIT_URL.replaceAll(/\.git$/,'')}/tree/${env.GIT_BRANCH}|${q env.BRANCH_NAME}>") +
    " $status"
}
